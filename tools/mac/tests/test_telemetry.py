import struct

import pytest

from app.config import SerialConfig
from app.telemetry_client import (PACKET_HEADER, PACKET_SIZE, TelemetryClient,
                                  decode_packet)


def test_decode_little_endian_packet_and_scaling() -> None:
    payload = struct.pack("<2sH" + "hhB" * 5, PACKET_HEADER, 65535,
                          -45, 900, 1, -40, 910, 1, -35, 920, 0,
                          -30, 930, 0, -25, 940, 0)
    decoded = decode_packet(payload, received_at=100.0)
    assert len(payload) == PACKET_SIZE == 29
    assert decoded["packet_sequence"] == 65535
    assert decoded["samples"][0]["speed"] == -4.5
    assert decoded["samples"][0]["can_position"] == 90.0
    assert decoded["samples"][0]["flash"] == 1
    assert decoded["samples"][-1]["flash"] == 0
    assert decoded["samples"][-1]["timestamp"] == 100.0
    assert decoded["samples"][1]["timestamp"] - decoded["samples"][0]["timestamp"] == pytest.approx(1 / 60)


def test_decode_rejects_partial_packet() -> None:
    with pytest.raises(ValueError):
        decode_packet(bytes(PACKET_SIZE - 1))


def test_decode_rejects_invalid_header() -> None:
    payload = struct.pack("<2sH" + "hhB" * 5, b"\x00\x00", 0, *([0] * 15))
    with pytest.raises(ValueError, match="header"):
        decode_packet(payload)


def _packet(sequence: int) -> bytes:
    return struct.pack("<2sH" + "hhB" * 5, PACKET_HEADER, sequence, *([0] * 15))


def test_drop_counter_handles_gap_and_sequence_restart() -> None:
    client = TelemetryClient(SerialConfig(port=""))
    try:
        client._accept_packet(_packet(100))
        client._accept_packet(_packet(103))
        assert client.snapshot()["dropped_packets"] == 2
        client._accept_packet(_packet(4))
        assert client.snapshot()["dropped_packets"] == 0
    finally:
        client.close()
