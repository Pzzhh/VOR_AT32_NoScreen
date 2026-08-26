#!/usr/bin/env python3
"""Small TCP simulator for the MCU Ctrl:VOR line protocol."""

from __future__ import annotations

import argparse
import re
import socketserver
import threading
import time


COMMAND = re.compile(r"^Ctrl:VOR\.(\w+) ID=(\d+)(?:\s+(.*))?$")


class DeviceState:
    def __init__(self) -> None:
        self.lock = threading.RLock()
        self.state = "IDLE"
        self.count = 0
        self.remaining = 0
        self.sequence = 0
        self.generation = 0
        self.encoder_angle = 0.0


DEVICE = DeviceState()


class Handler(socketserver.StreamRequestHandler):
    def send_line(self, line: str) -> None:
        with DEVICE.lock:
            self.wfile.write((line + "\r\n").encode("ascii"))
            self.wfile.flush()
        print(f"< {line}", flush=True)

    def event(self, event: str, state: str, progress: int, remaining: int,
              angle: float = None) -> None:
        with DEVICE.lock:
            DEVICE.sequence += 1
            sequence = DEVICE.sequence
        suffix = f" ANGLE={angle:.3f}" if angle is not None else ""
        self.send_line(f"Ctrl:EVT SEQ={sequence} EVENT={event} MODE=VOR "
                       f"STATE={state} CODE=0 PCT={progress} REM={remaining}{suffix}")

    def ack(self, request_id: int, action: str, state: str,
            accepted: bool = True, code: int = 0, message: str = "OK") -> None:
        kind = "ACK" if accepted else "NACK"
        self.send_line(f"Ctrl:{kind} ID={request_id} CMD=VOR.{action} CODE={code} "
                       f"STATE={state} MSG={message}")

    def run_task(self, generation: int) -> None:
        next_encoder = time.monotonic() + 1.0
        while True:
            time.sleep(0.2)
            with DEVICE.lock:
                if generation != DEVICE.generation or DEVICE.state == "IDLE":
                    return
                if DEVICE.state == "PAUSED":
                    continue
                DEVICE.remaining -= 1
                DEVICE.encoder_angle = (DEVICE.encoder_angle + 7.5) % 360.0
                remaining = max(0, DEVICE.remaining)
                progress = int((DEVICE.count - remaining) * 100 / DEVICE.count)
                completed = remaining == 0
                if completed:
                    DEVICE.state = "IDLE"
            if completed:
                self.event("COMPLETED", "READY", 100, 0)
                self.event("IDLE", "IDLE", 0, 0)
                return
            self.event("PROGRESS", "RUNNING", progress, remaining)
            if time.monotonic() >= next_encoder:
                next_encoder += 1.0
                self.event("ENCODER", "RUNNING", progress, remaining,
                           DEVICE.encoder_angle)

    def handle_command(self, line: str) -> None:
        match = COMMAND.match(line)
        if not match:
            self.send_line("Ctrl:NACK ID=0 CMD=UNKNOWN CODE=1001 STATE=IDLE MSG=INVALID_PARAM")
            return
        action, request_text, parameters = match.groups()
        request_id = int(request_text)
        with DEVICE.lock:
            state = DEVICE.state
            if action in {"Start", "Strat"}:
                if state != "IDLE":
                    self.ack(request_id, action, state, False, 1004, "BUSY")
                    return
                values = dict(item.split("=", 1) for item in (parameters or "").split()
                              if "=" in item)
                try:
                    DEVICE.count = int(values["C"])
                except (KeyError, ValueError):
                    self.ack(request_id, action, state, False, 1001, "INVALID_PARAM")
                    return
                DEVICE.remaining = DEVICE.count
                DEVICE.state = "RUNNING"
                DEVICE.generation += 1
                generation = DEVICE.generation
                self.ack(request_id, action, "STARTING", message="STARTED")
                self.event("RUNNING", "RUNNING", 0, DEVICE.remaining)
                threading.Thread(target=self.run_task, args=(generation,), daemon=True).start()
                return
            if action == "Status":
                progress = (int((DEVICE.count - DEVICE.remaining) * 100 / DEVICE.count)
                            if DEVICE.count else 0)
                self.ack(request_id, action, state, message="STATUS")
                self.event("STATUS", state, progress, DEVICE.remaining)
                return
            if action == "Pause" and state == "RUNNING":
                DEVICE.state = "PAUSED"
                self.ack(request_id, action, "PAUSED")
                self.event("PAUSED", "PAUSED", 0, DEVICE.remaining)
                return
            if action == "Resume" and state == "PAUSED":
                DEVICE.state = "RUNNING"
                self.ack(request_id, action, "RUNNING")
                self.event("RUNNING", "RUNNING", 0, DEVICE.remaining)
                return
            if action == "Stop" and state in {"RUNNING", "PAUSED"}:
                DEVICE.state = "IDLE"
                DEVICE.generation += 1
                self.ack(request_id, action, "STOPPING")
                self.event("STOPPED", "READY", 0, DEVICE.remaining)
                self.event("IDLE", "IDLE", 0, 0)
                return
            self.ack(request_id, action, state, False, 1003, "INVALID_STATE")

    def handle(self) -> None:
        print(f"client connected: {self.client_address}", flush=True)
        while True:
            raw = self.rfile.readline(512)
            if not raw:
                return
            line = raw.decode("ascii", errors="replace").strip()
            if line:
                print(f"> {line}", flush=True)
                self.handle_command(line)


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main() -> None:
    parser = argparse.ArgumentParser(description="VOR TCP device simulator")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9000)
    args = parser.parse_args()
    with Server((args.host, args.port), Handler) as server:
        print(f"VOR simulator listening on {args.host}:{args.port}", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
