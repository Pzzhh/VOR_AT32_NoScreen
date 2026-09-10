from __future__ import annotations

import queue
import threading
import time
from collections import deque
from dataclasses import asdict, dataclass
from datetime import datetime
from typing import Any, Callable, Deque, Dict, List, Optional

import serial
from serial.tools import list_ports

from .config import SerialConfig
from .protocol import Ack, Event, format_command, parse_line


class SerialClientError(RuntimeError):
    pass


class CommandTimeout(SerialClientError):
    pass


class CommandRejected(SerialClientError):
    def __init__(self, ack: Ack) -> None:
        super().__init__(f"{ack.command}: {ack.message} ({ack.code})")
        self.ack = ack


@dataclass
class PendingCommand:
    event: threading.Event
    response: Optional[Ack] = None
    error: Optional[str] = None


class SerialClient:
    """Single-owner USB serial worker with ACK matching and bounded logs."""

    def __init__(self, config: SerialConfig, log_limit: int = 1000) -> None:
        self.config = config
        self._operations: "queue.Queue[tuple]" = queue.Queue()
        self._pending: Dict[int, PendingCommand] = {}
        self._listeners: List[Callable[[str, Any], None]] = []
        self._logs: Deque[Dict[str, Any]] = deque(maxlen=log_limit)
        self._lock = threading.RLock()
        self._connected = False
        self._desired_connected = False
        self._connection_error = ""
        self._next_id = 1
        self._log_sequence = 0
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._worker, name="usb-transport", daemon=True)
        self._thread.start()

    def add_listener(self, callback: Callable[[str, Any], None]) -> None:
        self._listeners.append(callback)

    def connect(self) -> None:
        if not self.config.port:
            raise SerialClientError("serial.port is empty; edit config.json")
        self._desired_connected = True
        self._operations.put(("connect",))

    @staticmethod
    def available_ports() -> List[Dict[str, str]]:
        """List ports for the web selector, including Windows COM ports."""
        return [
            {"device": item.device,
             "label": " - ".join(part for part in
                               (item.device, item.description) if part),
             "description": item.description or "", "hwid": item.hwid or ""}
            for item in sorted(list_ports.comports(), key=lambda item: item.device)
        ]

    def select_port(self, port: str) -> None:
        if self._connected or self._desired_connected:
            raise SerialClientError("disconnect USB before selecting another serial port")
        selected = port.strip()
        if not selected:
            raise SerialClientError("serial port is empty")
        self.config = SerialConfig(selected, self.config.baudrate,
                                   self.config.ack_timeout_seconds,
                                   self.config.run_timeout_seconds,
                                   self.config.reconnect_interval_seconds)

    def disconnect(self) -> None:
        self._desired_connected = False
        self._operations.put(("disconnect",))

    def close(self) -> None:
        self._stop.set()
        self._operations.put(("shutdown",))
        self._thread.join(timeout=2)

    def command(self, action: str, parameters: str = "",
                timeout: Optional[float] = None, mode: str = "VOR") -> Ack:
        with self._lock:
            if not self._connected:
                raise SerialClientError("USB serial is not connected")
            request_id = self._next_id
            self._next_id += 1
            pending = PendingCommand(threading.Event())
            self._pending[request_id] = pending
        self._operations.put(("send", request_id,
                              format_command(action, request_id, parameters, mode)))
        wait_time = timeout or self.config.ack_timeout_seconds
        if not pending.event.wait(wait_time):
            with self._lock:
                self._pending.pop(request_id, None)
            raise CommandTimeout(f"no ACK for ID={request_id}")
        if pending.error:
            raise SerialClientError(pending.error)
        if pending.response is None:
            raise SerialClientError("command completed without response")
        if not pending.response.accepted:
            raise CommandRejected(pending.response)
        return pending.response

    def snapshot(self) -> Dict[str, Any]:
        with self._lock:
            return {
                "connected": self._connected,
                "desired_connected": self._desired_connected,
                "port": self.config.port,
                "baudrate": self.config.baudrate,
                "transport": "serial",
                "endpoint": self.config.port,
                "error": self._connection_error,
            }

    def logs(self, offset: int = 0, limit: int = 200) -> Dict[str, Any]:
        with self._lock:
            items = list(self._logs)
        offset = max(0, offset)
        limit = max(1, min(limit, 500))
        return {"total": len(items), "items": items[offset:offset + limit]}

    def logs_after(self, sequence: int) -> List[Dict[str, Any]]:
        with self._lock:
            return [item for item in self._logs if item["sequence"] > sequence]

    def _notify(self, kind: str, payload: Any) -> None:
        for listener in tuple(self._listeners):
            try:
                listener(kind, payload)
            except Exception as error:
                self._add_log("ERROR", f"listener error: {error}", "ERROR")

    def _add_log(self, direction: str, raw: str, kind: str) -> None:
        with self._lock:
            self._log_sequence += 1
            item = {
                "sequence": self._log_sequence,
                "timestamp": datetime.now().isoformat(timespec="milliseconds"),
                "direction": direction,
                "kind": kind,
                "raw": raw,
            }
            self._logs.append(item)
        self._notify("log", item)

    def _set_connected(self, connected: bool, error: str = "") -> None:
        with self._lock:
            changed = self._connected != connected or self._connection_error != error
            self._connected = connected
            self._connection_error = error
        if changed:
            self._notify("connection", self.snapshot())

    def _fail_pending(self, reason: str) -> None:
        with self._lock:
            pending = list(self._pending.values())
            self._pending.clear()
        for item in pending:
            item.error = reason
            item.event.set()

    def _handle_line(self, line: str) -> None:
        parsed = parse_line(line)
        kind = "RX"
        if isinstance(parsed, Ack):
            kind = "ACK" if parsed.accepted else "NACK"
        elif isinstance(parsed, Event):
            kind = "EVT"
        self._add_log("RX", line, kind)
        if isinstance(parsed, Ack):
            with self._lock:
                pending = self._pending.pop(parsed.request_id, None)
            if pending:
                pending.response = parsed
                pending.event.set()
        if parsed is not None:
            self._notify("protocol", parsed)

    def _worker(self) -> None:
        device = None
        buffer = bytearray()
        next_reconnect = 0.0
        while not self._stop.is_set():
            try:
                operation = self._operations.get(timeout=0.02)
            except queue.Empty:
                operation = None
            if operation:
                name = operation[0]
                if name == "shutdown":
                    break
                if name == "disconnect":
                    if device:
                        device.close()
                    device = None
                    buffer.clear()
                    self._set_connected(False)
                    self._fail_pending("USB disconnected")
                elif name == "connect":
                    next_reconnect = 0.0
                elif name == "send":
                    request_id, payload = operation[1], operation[2]
                    if device is None:
                        with self._lock:
                            pending = self._pending.pop(request_id, None)
                        if pending:
                            pending.error = "USB is not connected"
                            pending.event.set()
                    else:
                        try:
                            device.write(payload)
                            device.flush()
                            self._add_log("TX", payload.decode("ascii").strip(), "TX")
                        except Exception as error:
                            self._add_log("ERROR", str(error), "ERROR")
                            device.close()
                            device = None
                            self._set_connected(False, str(error))
                            self._fail_pending(str(error))
                            next_reconnect = time.monotonic() + self.config.reconnect_interval_seconds

            if device is None and self._desired_connected and time.monotonic() >= next_reconnect:
                try:
                    device = serial.Serial(self.config.port, self.config.baudrate,
                                           timeout=0, write_timeout=1)
                    endpoint = self.config.port
                    buffer.clear()
                    self._set_connected(True)
                    self._add_log("INFO", f"connected USB serial {endpoint}", "INFO")
                except Exception as error:
                    self._set_connected(False, str(error))
                    next_reconnect = time.monotonic() + self.config.reconnect_interval_seconds

            if device is not None:
                try:
                    chunk = device.read(64)
                    if chunk:
                        buffer.extend(chunk)
                        while b"\n" in buffer or b"\r" in buffer:
                            positions = [p for p in (buffer.find(b"\r"), buffer.find(b"\n")) if p >= 0]
                            end = min(positions)
                            raw = bytes(buffer[:end])
                            consume = end + 1
                            while consume < len(buffer) and buffer[consume] in (10, 13):
                                consume += 1
                            del buffer[:consume]
                            line = raw.decode("ascii", errors="replace").strip()
                            if line:
                                self._handle_line(line)
                    if len(buffer) > 512:
                        self._add_log("ERROR", "RX line too long", "ERROR")
                        buffer.clear()
                except Exception as error:
                    self._add_log("ERROR", str(error), "ERROR")
                    device.close()
                    device = None
                    self._set_connected(False, str(error))
                    self._fail_pending(str(error))
                    next_reconnect = time.monotonic() + self.config.reconnect_interval_seconds
        if device:
            device.close()
        self._set_connected(False)
        self._fail_pending("serial worker stopped")
