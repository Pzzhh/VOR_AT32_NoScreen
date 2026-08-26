#!/usr/bin/env python3
"""Bidirectional TCP <-> TTY bridge for the Linux VOR simulator.

The bridge is intentionally protocol-transparent. Bytes received from the TCP
client are written to the configured TTY peer, and bytes read from the TTY are
sent back to the connected TCP client.
"""

from __future__ import annotations

import argparse
import select
import signal
import socket
import sys
import time
from typing import Optional

try:
    import serial
except ImportError:
    print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)


class Bridge:
    def __init__(self, host: str, port: int, tty: str, baudrate: int,
                 reconnect_seconds: float, verbose: bool) -> None:
        self.host = host
        self.port = port
        self.tty = tty
        self.baudrate = baudrate
        self.reconnect_seconds = reconnect_seconds
        self.verbose = verbose
        self.running = True
        self.serial_port: Optional[serial.Serial] = None
        self.client: Optional[socket.socket] = None

    def log(self, message: str) -> None:
        print(message, flush=True)

    def trace(self, direction: str, data: bytes) -> None:
        if not self.verbose:
            return
        preview = data.decode("ascii", errors="backslashreplace").rstrip()
        self.log(f"{direction} {len(data)} B: {preview}")

    def stop(self, _signum=None, _frame=None) -> None:
        self.running = False

    def open_serial(self) -> bool:
        if self.serial_port and self.serial_port.is_open:
            return True
        try:
            self.serial_port = serial.Serial(
                self.tty,
                self.baudrate,
                timeout=0,
                write_timeout=1,
            )
            self.log(f"TTY connected: {self.tty} @ {self.baudrate}")
            return True
        except (OSError, serial.SerialException) as error:
            self.log(f"TTY unavailable: {error}; retrying in {self.reconnect_seconds:g}s")
            self.serial_port = None
            return False

    def close_client(self) -> None:
        if self.client:
            try:
                self.client.close()
            except OSError:
                pass
            self.client = None
            self.log("TCP client disconnected")

    def close_serial(self) -> None:
        if self.serial_port:
            try:
                self.serial_port.close()
            except (OSError, serial.SerialException):
                pass
            self.serial_port = None

    def bridge_client(self) -> None:
        assert self.client is not None
        while self.running and self.client is not None:
            if not self.open_serial():
                time.sleep(self.reconnect_seconds)
                continue
            assert self.serial_port is not None
            try:
                readable, _, _ = select.select(
                    [self.client, self.serial_port.fileno()], [], [], 0.25)
                if self.client in readable:
                    data = self.client.recv(4096)
                    if not data:
                        return
                    self.trace("TCP->TTY", data)
                    self.serial_port.write(data)
                    self.serial_port.flush()
                if self.serial_port.fileno() in readable:
                    data = self.serial_port.read(4096)
                    if data:
                        self.trace("TTY->TCP", data)
                        self.client.sendall(data)
            except serial.SerialException as error:
                self.log(f"TTY bridge error: {error}")
                self.close_serial()
                time.sleep(self.reconnect_seconds)
            except OSError as error:
                self.log(f"TCP bridge error: {error}")
                return

    def run(self) -> None:
        signal.signal(signal.SIGINT, self.stop)
        signal.signal(signal.SIGTERM, self.stop)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind((self.host, self.port))
            listener.listen(1)
            listener.settimeout(0.5)
            self.log(f"TCP listening: {self.host}:{self.port}")
            self.log(f"TTY target: {self.tty}")
            while self.running:
                try:
                    client, address = listener.accept()
                except socket.timeout:
                    continue
                except OSError:
                    if self.running:
                        raise
                    break
                self.close_client()
                self.client = client
                client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                self.log(f"TCP client connected: {address[0]}:{address[1]}")
                try:
                    self.bridge_client()
                finally:
                    self.close_client()
            self.close_serial()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Bidirectional TCP to Linux TTY/PTY bridge")
    parser.add_argument("--tty", required=True,
                        help="TTY peer path, e.g. /tmp/ttyUSBpeer")
    parser.add_argument("--host", default="0.0.0.0",
                        help="TCP listen address (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=9000,
                        help="TCP listen port (default: 9000)")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--reconnect-seconds", type=float, default=1.0)
    parser.add_argument("--verbose", action="store_true",
                        help="print forwarded payloads")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")
    if args.baudrate <= 0:
        parser.error("--baudrate must be positive")
    if args.reconnect_seconds <= 0:
        parser.error("--reconnect-seconds must be positive")
    return args


def main() -> int:
    args = parse_args()
    bridge = Bridge(args.host, args.port, args.tty, args.baudrate,
                    args.reconnect_seconds, args.verbose)
    try:
        bridge.run()
    except (OSError, serial.SerialException) as error:
        print(f"fatal: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
