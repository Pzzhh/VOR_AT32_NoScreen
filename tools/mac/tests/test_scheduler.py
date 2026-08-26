from __future__ import annotations

from dataclasses import dataclass

import pytest

from app.models import VorTaskInput
from app.protocol import Ack, Event
from app.scheduler import SchedulerError, TaskScheduler
from app.storage import HistoryStore


@dataclass
class FakeConfig:
    ack_timeout_seconds: float = 0.1


class FakeClient:
    def __init__(self) -> None:
        self.config = FakeConfig()
        self.listeners = []
        self.commands = []

    def add_listener(self, callback):
        self.listeners.append(callback)

    def command(self, action, parameters="", timeout=None):
        self.commands.append((action, parameters))
        return Ack(True, len(self.commands), f"VOR.{action}", 0,
                   "RUNNING" if action != "Stop" else "STOPPING", "OK", "")

    def emit(self, event, state="RUNNING", progress=0, remaining=0, code=0):
        payload = Event(1, event, "VOR", state, code, progress, remaining, "")
        for listener in self.listeners:
            listener("protocol", payload)


def make_task(name="Task"):
    return VorTaskInput(count=20, velocity=50,
                        frequency=1.6, ex_mode=0)


def test_task_limit() -> None:
    scheduler = TaskScheduler(FakeClient())
    try:
        for index in range(20):
            scheduler.add_task(make_task(str(index)))
        with pytest.raises(SchedulerError) as error:
            scheduler.add_task(make_task("overflow"))
        assert error.value.code == "TASK_LIMIT"
    finally:
        scheduler.close()


def test_sequential_tasks() -> None:
    client = FakeClient()
    scheduler = TaskScheduler(client)
    try:
        scheduler.add_task(make_task("one"))
        scheduler.add_task(make_task("two"))
        scheduler.execute("start")
        client.emit("RUNNING")
        client.emit("COMPLETED", state="READY", progress=100)
        client.emit("IDLE", state="IDLE", progress=0)
        scheduler.execute("status")  # queue barrier
        assert len([item for item in client.commands if item[0] == "Start"]) == 2
        client.emit("COMPLETED", state="READY", progress=100)
        client.emit("IDLE", state="IDLE")
        scheduler.execute("status")
        assert scheduler.snapshot()["state"] == "COMPLETED"
    finally:
        scheduler.close()


def test_running_list_is_locked() -> None:
    client = FakeClient()
    scheduler = TaskScheduler(client)
    try:
        scheduler.add_task(make_task())
        scheduler.execute("start")
        with pytest.raises(SchedulerError) as error:
            scheduler.add_task(make_task("blocked"))
        assert error.value.code == "RUN_ACTIVE"
    finally:
        scheduler.close()


def test_encoder_event_updates_snapshot() -> None:
    client = FakeClient()
    scheduler = TaskScheduler(client)
    try:
        scheduler.add_task(make_task())
        scheduler.execute("start")
        payload = Event(2, "ENCODER", "VOR", "RUNNING", 0, 10, 18,
                        "", 42.125)
        for listener in client.listeners:
            listener("protocol", payload)
        scheduler.execute("status")
        assert scheduler.snapshot()["encoder_angle"] == 42.125
    finally:
        scheduler.close()


def test_start_from_selected_index_and_history(tmp_path) -> None:
    client = FakeClient()
    history = HistoryStore(tmp_path / "history.json", 20)
    scheduler = TaskScheduler(client, history)
    try:
        scheduler.add_task(make_task())
        scheduler.add_task(make_task())
        scheduler.add_task(make_task())
        scheduler.execute("start", 1)
        state = scheduler.snapshot()
        assert state["tasks"][0]["state"] == "SKIPPED"
        assert state["current_index"] == 1
        client.emit("COMPLETED", state="READY", progress=100)
        client.emit("IDLE", state="IDLE")
        scheduler.execute("status")
        client.emit("COMPLETED", state="READY", progress=100)
        client.emit("IDLE", state="IDLE")
        scheduler.execute("status")
        record = history.list()[0]
        assert record["start_index"] == 1
        assert record["result"] == "COMPLETED"
        assert record["ended_at"] is not None
    finally:
        scheduler.close()


def test_run_only_selected_task() -> None:
    client = FakeClient()
    scheduler = TaskScheduler(client)
    try:
        scheduler.add_task(make_task())
        scheduler.add_task(make_task())
        scheduler.add_task(make_task())
        scheduler.execute("start", {"start_index": 1, "run_only": True})
        client.emit("COMPLETED", state="READY", progress=100)
        client.emit("IDLE", state="IDLE")
        scheduler.execute("status")
        starts = [item for item in client.commands if item[0] == "Start"]
        assert len(starts) == 1
        state = scheduler.snapshot()
        assert state["state"] == "COMPLETED"
        assert state["tasks"][0]["state"] == "SKIPPED"
        assert state["tasks"][1]["state"] == "COMPLETED"
        assert state["tasks"][2]["state"] == "QUEUED"
    finally:
        scheduler.close()
