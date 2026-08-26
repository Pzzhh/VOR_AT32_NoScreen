#!/usr/bin/env python3
"""Exercise the Ctrl:VOR USB text protocol."""

from __future__ import annotations

import argparse
import queue
import re
import sys
import threading
import time

try:
    import serial
except ImportError:
    print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)
if not hasattr(serial, "Serial"):
    print(
        "pyserial is required (a different 'serial' package is installed): "
        "python3 -m pip install pyserial",
        file=sys.stderr,
    )
    raise SystemExit(2)


ACK_RE = re.compile(
    r"^Ctrl:(ACK|NACK) ID=(\d+) CMD=(\S+) CODE=(\d+) STATE=(\S+) MSG=(\S+)"
)
EVT_RE = re.compile(
    r"^Ctrl:EVT SEQ=(\d+) EVENT=(\S+) MODE=(\S+) STATE=(\S+) "
    r"CODE=(\d+) PCT=(\d+) REM=(\d+)"
)


class VorClient:
    def __init__(self, port: str, baudrate: int, timeout: float, mode: str) -> None:
        self.serial = serial.Serial(port, baudrate, timeout=0.1)
        self.timeout = timeout
        self.lines: queue.Queue[str] = queue.Queue()
        self.stop_reader = threading.Event()
        self.reader = threading.Thread(target=self._reader, daemon=True)
        self.next_id = 1
        self.mode = mode
        self.reader.start()

    def close(self) -> None:
        self.stop_reader.set()
        self.reader.join(timeout=1)
        self.serial.close()

    def _reader(self) -> None:
        while not self.stop_reader.is_set():
            raw = self.serial.readline()
            if not raw:
                continue
            line = raw.decode("ascii", errors="replace").strip()
            if line:
                print(f"< {line}")
                self.lines.put(line)

    def send(self, verb: str, parameters: str = "") -> int:
        request_id = self.next_id
        self.next_id += 1
        suffix = f" {parameters}" if parameters else ""
        line = f"Ctrl:{self.mode}.{verb} ID={request_id}{suffix}\r\n"
        print(f"> {line.rstrip()}")
        self.serial.write(line.encode("ascii"))
        self.serial.flush()
        return request_id

    def wait_ack(self, request_id: int) -> re.Match[str]:
        deadline = time.monotonic() + self.timeout
        deferred: list[str] = []
        try:
            while time.monotonic() < deadline:
                try:
                    line = self.lines.get(timeout=0.1)
                except queue.Empty:
                    continue
                match = ACK_RE.match(line)
                if match and int(match.group(2)) == request_id:
                    if match.group(1) != "ACK":
                        raise RuntimeError(f"command rejected: {line}")
                    return match
                deferred.append(line)
        finally:
            for line in deferred:
                self.lines.put(line)
        raise TimeoutError(f"no ACK for ID={request_id}")

    def wait_event(self, event: str, timeout: float | None = None) -> re.Match[str]:
        deadline = time.monotonic() + (timeout or self.timeout)
        deferred: list[str] = []
        try:
            while time.monotonic() < deadline:
                try:
                    line = self.lines.get(timeout=0.1)
                except queue.Empty:
                    continue
                match = EVT_RE.match(line)
                if match and match.group(2) == event:
                    return match
                deferred.append(line)
        finally:
            for line in deferred:
                self.lines.put(line)
        raise TimeoutError(f"no {event} event")

    def wait_progress(self, threshold: int) -> int:
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=0.1)
            except queue.Empty:
                continue
            match = EVT_RE.match(line)
            if match and match.group(2) == "PROGRESS":
                progress = int(match.group(6))
                if progress >= threshold:
                    return progress
        raise TimeoutError(f"progress did not reach {threshold}%")


def command(client: VorClient, verb: str, parameters: str = "") -> None:
    client.wait_ack(client.send(verb, parameters))


def interactive(client: VorClient, start_parameters: str) -> None:
    print("Commands: strat, pause, resume, stop, status, next, exit, quit")
    while True:
        verb = input("vor> ").strip().lower()
        if verb in {"quit", "q"}:
            return
        if verb not in {"strat", "start", "pause", "resume", "stop", "status", "next", "exit"}:
            print("unknown local command")
            continue
        parameters = start_parameters if verb in {"strat", "start"} else ""
        try:
            command(client, verb.capitalize(), parameters)
        except (RuntimeError, TimeoutError) as error:
            print(error, file=sys.stderr)


def automatic(client: VorClient, args: argparse.Namespace) -> None:
    parameters = start_parameters(args)
    command(client, "Strat", parameters)
    client.wait_event("RUNNING")
    command(client, "Status")

    if args.pause_at is not None:
        client.wait_progress(args.pause_at)
        command(client, "Pause")
        client.wait_event("PAUSED")
        time.sleep(args.pause_seconds)
        command(client, "Resume")
        client.wait_event("RUNNING")

    if args.stop_at is not None:
        client.wait_progress(args.stop_at)
        command(client, "Stop")
        client.wait_event("IDLE")
    else:
        client.wait_event("COMPLETED", timeout=args.run_timeout)
        client.wait_event("IDLE")
    print("USB VOR test passed")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--mode", choices=("VOR", "CONT", "OVAR", "VHIT", "TC"),
                        default="VOR")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--velocity", type=float, default=50)
    parser.add_argument("--frequency", type=float, default=1.6)
    parser.add_argument("--ex-mode", type=int, choices=range(4), default=0)
    parser.add_argument("--seconds", type=int, default=3)
    parser.add_argument("--okr-velocity", type=float, default=0)
    parser.add_argument("--incline", type=float, default=10)
    parser.add_argument("--direction", type=int, choices=(0, 1), default=1,
                        help="VHIT direction: 0=CCW, 1=CW")
    parser.add_argument("--pause-at", type=int)
    parser.add_argument("--pause-seconds", type=float, default=1.0)
    parser.add_argument("--stop-at", type=int)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--run-timeout", type=float, default=120.0)
    parser.add_argument("--interactive", action="store_true")
    return parser.parse_args()


def start_parameters(args: argparse.Namespace) -> str:
    if args.mode == "VOR":
        return (f"C={args.count} V={args.velocity:g} "
                f"F={args.frequency:g} E={args.ex_mode}")
    if args.mode == "CONT":
        return (f"S={args.seconds} V={args.velocity:g} "
                f"O={args.okr_velocity:g}")
    if args.mode == "OVAR":
        return (f"S={args.seconds} V={args.velocity:g} "
                f"I={args.incline:g}")
    if args.mode == "VHIT":
        return f"C={args.count} D={args.direction}"
    return f"C={args.count} V={args.velocity:g}"


def main() -> int:
    args = parse_args()
    if not 1 <= args.count <= 1000:
        raise SystemExit("--count must be 1..1000")
    if not -360 <= args.velocity <= 360:
        raise SystemExit("--velocity must be -360..360")
    if not 0 < args.frequency <= 11:
        raise SystemExit("--frequency must be >0 and <=11")
    if not 1 <= args.seconds <= 1000:
        raise SystemExit("--seconds must be 1..1000")
    if not -360 <= args.okr_velocity <= 360:
        raise SystemExit("--okr-velocity must be -360..360")
    if not 0 <= args.incline <= 50:
        raise SystemExit("--incline must be 0..50")
    if args.mode == "VHIT" and (args.pause_at is not None):
        raise SystemExit("VHIT does not support Pause/Resume; use Next interactively")

    client = VorClient(args.port, args.baudrate, args.timeout, args.mode)
    parameters = start_parameters(args)
    try:
        if args.interactive:
            interactive(client, parameters)
        else:
            automatic(client, args)
    finally:
        client.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
