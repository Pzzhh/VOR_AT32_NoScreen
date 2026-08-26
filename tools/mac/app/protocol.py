from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Optional, Union


ACK_RE = re.compile(
    r"^Ctrl:(ACK|NACK) ID=(\d+) CMD=(\S+) CODE=(\d+) STATE=(\S+) MSG=(\S+)"
)
EVT_RE = re.compile(
    r"^Ctrl:EVT SEQ=(\d+) EVENT=(\S+) MODE=(\S+) STATE=(\S+) "
    r"CODE=(\d+) PCT=(\d+) REM=(\d+)(?: ANGLE=(-?\d+(?:\.\d+)?))?"
)


@dataclass(frozen=True)
class Ack:
    accepted: bool
    request_id: int
    command: str
    code: int
    state: str
    message: str
    raw: str


@dataclass(frozen=True)
class Event:
    sequence: int
    event: str
    mode: str
    state: str
    code: int
    progress: int
    remaining: int
    raw: str
    angle: Optional[float] = None


def parse_line(line: str) -> Optional[Union[Ack, Event]]:
    ack = ACK_RE.match(line)
    if ack:
        return Ack(ack.group(1) == "ACK", int(ack.group(2)), ack.group(3),
                   int(ack.group(4)), ack.group(5), ack.group(6), line)
    event = EVT_RE.match(line)
    if event:
        return Event(int(event.group(1)), event.group(2), event.group(3),
                     event.group(4), int(event.group(5)), int(event.group(6)),
                     int(event.group(7)), line,
                     float(event.group(8)) if event.group(8) is not None else None)
    return None


def format_command(action: str, request_id: int, parameters: str = "") -> bytes:
    suffix = f" {parameters}" if parameters else ""
    return f"Ctrl:VOR.{action} ID={request_id}{suffix}\r\n".encode("ascii")
