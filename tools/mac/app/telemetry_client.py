from __future__ import annotations

import struct
import threading
import time
from collections import deque
from typing import Any, Deque, Dict, List

import serial

from .config import SerialConfig


PACKET_HEADER = b"\xAA\x56"
PACKET_SIZE = 29
SAMPLES_PER_PACKET = 5
SAMPLE_PERIOD_SECONDS = 1.0 / 60.0
_PACKET = struct.Struct("<2sH" + "hhB" * SAMPLES_PER_PACKET)


def decode_packet(payload: bytes, received_at: float | None = None) -> Dict[str, Any]:
    if len(payload) != PACKET_SIZE:
        raise ValueError(f"telemetry packet must be {PACKET_SIZE} bytes")
    values = _PACKET.unpack(payload)
    if values[0] != PACKET_HEADER:
        raise ValueError("invalid telemetry packet header")
    timestamp = time.time() if received_at is None else received_at
    samples = []
    for index in range(SAMPLES_PER_PACKET):
        flash = values[4 + index * 3]
        if flash not in (0, 1):
            raise ValueError("invalid flash state")
        samples.append({
            "timestamp": timestamp - (SAMPLES_PER_PACKET - 1 - index) * SAMPLE_PERIOD_SECONDS,
            "speed": values[2 + index * 3] / 10.0,
            "can_position": values[3 + index * 3] / 10.0,
            "flash": flash,
        })
    return {"packet_sequence": values[1], "samples": samples}


class TelemetryClient:
    """Owns CDC2 and exposes decoded 60 Hz motor samples."""

    def __init__(self, config: SerialConfig, sample_limit: int = 36000) -> None:
        self.config = config
        self._samples: Deque[Dict[str, Any]] = deque(maxlen=sample_limit)
        self._lock = threading.RLock()
        self._desired_connected = False
        self._connected = False
        self._error = ""
        self._sample_sequence = 0
        self._last_packet_sequence: int | None = None
        self._dropped_packets = 0
        self._stop = threading.Event()
        self._wake = threading.Event()
        self._thread = threading.Thread(target=self._worker,
                                        name="usb-telemetry", daemon=True)
        self._thread.start()

    def select_port(self, port: str) -> None:
        with self._lock:
            if self._connected or self._desired_connected:
                raise RuntimeError("disconnect USB before selecting telemetry port")
            self.config = SerialConfig(port.strip(), self.config.baudrate,
                                       self.config.ack_timeout_seconds,
                                       self.config.run_timeout_seconds,
                                       self.config.reconnect_interval_seconds)

    def connect(self) -> None:
        if not self.config.port:
            raise RuntimeError("telemetry serial port is empty")
        self._desired_connected = True
        self._wake.set()

    def disconnect(self) -> None:
        self._desired_connected = False
        self._wake.set()

    def close(self) -> None:
        self._stop.set()
        self._wake.set()
        self._thread.join(timeout=2)

    def snapshot(self) -> Dict[str, Any]:
        with self._lock:
            latest = self._samples[-1] if self._samples else None
            return {
                "connected": self._connected,
                "desired_connected": self._desired_connected,
                "port": self.config.port,
                "baudrate": self.config.baudrate,
                "error": self._error,
                "latest": latest,
                "dropped_packets": self._dropped_packets,
            }

    def samples_after(self, sequence: int) -> List[Dict[str, Any]]:
        with self._lock:
            return [sample for sample in self._samples
                    if sample["sequence"] > sequence]

    def samples_snapshot(self) -> List[Dict[str, Any]]:
        with self._lock:
            return list(self._samples)

    def _set_state(self, connected: bool, error: str = "") -> None:
        with self._lock:
            self._connected = connected
            self._error = error

    def _accept_packet(self, payload: bytes) -> None:
        decoded = decode_packet(payload)
        packet_sequence = decoded["packet_sequence"]
        with self._lock:
            if self._last_packet_sequence is not None:
                delta = (packet_sequence - self._last_packet_sequence) & 0xFFFF
                if packet_sequence == 0 and self._last_packet_sequence != 0xFFFF:
                    self._dropped_packets = 0
                elif 1 < delta < 0x8000:
                    self._dropped_packets += delta - 1
                elif delta >= 0x8000:
                    # A backwards jump means task/device restart, not 65k losses.
                    self._dropped_packets = 0
            self._last_packet_sequence = packet_sequence
            for sample in decoded["samples"]:
                self._sample_sequence += 1
                sample["sequence"] = self._sample_sequence
                sample["packet_sequence"] = packet_sequence
                self._samples.append(sample)

    def _worker(self) -> None:
        device = None
        buffer = bytearray()
        next_reconnect = 0.0
        while not self._stop.is_set():
            if not self._desired_connected:
                if device is not None:
                    device.close()
                    device = None
                buffer.clear()
                self._last_packet_sequence = None
                self._set_state(False)
                self._wake.wait(0.2)
                self._wake.clear()
                continue
            if device is None and time.monotonic() >= next_reconnect:
                try:
                    device = serial.Serial(self.config.port, self.config.baudrate,
                                           timeout=0.1, write_timeout=1)
                    buffer.clear()
                    self._last_packet_sequence = None
                    self._set_state(True)
                except Exception as error:
                    self._set_state(False, str(error))
                    next_reconnect = (time.monotonic() +
                                      self.config.reconnect_interval_seconds)
            if device is None:
                self._wake.wait(0.1)
                self._wake.clear()
                continue
            try:
                chunk = device.read(256)
                if chunk:
                    buffer.extend(chunk)
                    while True:
                        header_at = buffer.find(PACKET_HEADER)
                        if header_at < 0:
                            if buffer[-1:] != PACKET_HEADER[:1]:
                                buffer.clear()
                            elif len(buffer) > 1:
                                del buffer[:-1]
                            break
                        if header_at:
                            del buffer[:header_at]
                        if len(buffer) < PACKET_SIZE:
                            break
                        if any(buffer[8 + index * 5] not in (0, 1)
                               for index in range(SAMPLES_PER_PACKET)):
                            del buffer[0]
                            continue
                        self._accept_packet(bytes(buffer[:PACKET_SIZE]))
                        del buffer[:PACKET_SIZE]
            except Exception as error:
                try:
                    device.close()
                finally:
                    device = None
                buffer.clear()
                self._set_state(False, str(error))
                next_reconnect = (time.monotonic() +
                                  self.config.reconnect_interval_seconds)
        if device is not None:
            device.close()
        self._set_state(False)
