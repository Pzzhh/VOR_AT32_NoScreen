from __future__ import annotations

import queue
import threading
from dataclasses import dataclass
from typing import Any, Dict, List, Optional

from .models import RunState, TaskState, VorTask, VorTaskInput
from .protocol import Event
from .serial_client import CommandRejected, SerialClient, SerialClientError
from .storage import HistoryStore


class SchedulerError(RuntimeError):
    def __init__(self, message: str, code: str = "INVALID_STATE") -> None:
        super().__init__(message)
        self.code = code


@dataclass
class Action:
    name: str
    argument: Any
    done: threading.Event
    result: Any = None
    error: Optional[Exception] = None


class TaskScheduler:
    MAX_TASKS = 20
    MUTABLE_STATES = {RunState.IDLE, RunState.COMPLETED}

    def __init__(self, client: SerialClient,
                 history: Optional[HistoryStore] = None) -> None:
        self.client = client
        self.history = history
        self._tasks: List[VorTask] = []
        self._next_task_id = 1
        self._state = RunState.IDLE
        self._current_index: Optional[int] = None
        self._message = "ready"
        self._waiting_idle = False
        self._encoder_angle: Optional[float] = None
        self._stop_requested = False
        self._history_id: Optional[str] = None
        self._run_end_index: Optional[int] = None
        self._lock = threading.RLock()
        self._actions: "queue.Queue[Action]" = queue.Queue()
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._worker, name="task-scheduler", daemon=True)
        self.client.add_listener(self._serial_event)
        self._thread.start()

    def close(self) -> None:
        self._stop.set()
        self._actions.put(Action("shutdown", None, threading.Event()))
        self._thread.join(timeout=2)

    def snapshot(self) -> Dict[str, Any]:
        with self._lock:
            current = None
            if self._current_index is not None and self._current_index < len(self._tasks):
                current = self._tasks[self._current_index].to_dict(self._current_index)
            return {
                "state": self._state.value,
                "message": self._message,
                "current_index": self._current_index,
                "current_task": current,
                "task_count": len(self._tasks),
                "tasks": [task.to_dict(index) for index, task in enumerate(self._tasks)],
                "encoder_angle": self._encoder_angle,
            }

    def tasks(self) -> List[Dict[str, Any]]:
        return self.snapshot()["tasks"]

    def add_task(self, data: VorTaskInput) -> Dict[str, Any]:
        with self._lock:
            self._require_mutable()
            if len(self._tasks) >= self.MAX_TASKS:
                raise SchedulerError("task list is full", "TASK_LIMIT")
            task = VorTask.from_input(self._next_task_id, data)
            self._next_task_id += 1
            self._tasks.append(task)
            return task.to_dict(len(self._tasks) - 1)

    def update_task(self, task_id: int, data: VorTaskInput) -> Dict[str, Any]:
        with self._lock:
            self._require_mutable()
            task = self._find_task(task_id)
            task.update_from(data)
            return task.to_dict(self._tasks.index(task))

    def delete_task(self, task_id: int) -> None:
        with self._lock:
            self._require_mutable()
            task = self._find_task(task_id)
            self._tasks.remove(task)

    def reorder(self, ids: List[int]) -> None:
        with self._lock:
            self._require_mutable()
            existing = [task.id for task in self._tasks]
            if len(ids) != len(existing) or set(ids) != set(existing):
                raise SchedulerError("ids must contain every task exactly once", "INVALID_ARGUMENT")
            by_id = {task.id: task for task in self._tasks}
            self._tasks = [by_id[task_id] for task_id in ids]

    def replace_tasks(self, task_inputs: List[VorTaskInput]) -> List[Dict[str, Any]]:
        with self._lock:
            self._require_mutable()
            if not task_inputs or len(task_inputs) > self.MAX_TASKS:
                raise SchedulerError("template must contain 1..20 tasks",
                                     "INVALID_ARGUMENT")
            self._tasks = []
            for data in task_inputs:
                self._tasks.append(VorTask.from_input(self._next_task_id, data))
                self._next_task_id += 1
            return [task.to_dict(index) for index, task in enumerate(self._tasks)]

    def template_tasks(self) -> List[Dict[str, Any]]:
        with self._lock:
            self._require_mutable()
            return [{"count": task.count, "velocity": task.velocity,
                     "frequency": task.frequency, "ex_mode": task.ex_mode}
                    for task in self._tasks]

    def is_mutable(self) -> bool:
        with self._lock:
            return self._state in self.MUTABLE_STATES

    def _history_update(self, result: str, terminal: bool = False) -> None:
        if self.history is None or self._history_id is None:
            return
        tasks = [task.to_dict(index) for index, task in enumerate(self._tasks)]
        self.history.update(self._history_id, result, tasks, terminal)

    def execute(self, name: str, argument: Any = None, timeout: float = 10.0) -> Any:
        action = Action(name, argument, threading.Event())
        self._actions.put(action)
        if not action.done.wait(timeout):
            raise SchedulerError(f"scheduler action timed out: {name}", "TIMEOUT")
        if action.error:
            if isinstance(action.error, SchedulerError):
                raise action.error
            raise SchedulerError(str(action.error), "SERIAL_ERROR")
        return action.result

    def _require_mutable(self) -> None:
        if self._state not in self.MUTABLE_STATES:
            raise SchedulerError("task list is locked while a run is active", "RUN_ACTIVE")

    def _find_task(self, task_id: int) -> VorTask:
        for task in self._tasks:
            if task.id == task_id:
                return task
        raise SchedulerError("task not found", "NOT_FOUND")

    def _serial_event(self, kind: str, payload: Any) -> None:
        if kind == "protocol" and isinstance(payload, Event):
            self._actions.put(Action("device_event", payload, threading.Event()))
        elif kind == "connection" and not payload.get("connected", False):
            self._actions.put(Action("disconnected", payload, threading.Event()))

    def _worker(self) -> None:
        while not self._stop.is_set():
            action = self._actions.get()
            if action.name == "shutdown":
                action.done.set()
                break
            try:
                action.result = self._dispatch(action.name, action.argument)
            except Exception as error:
                action.error = error
            finally:
                action.done.set()

    def _dispatch(self, name: str, argument: Any) -> Any:
        if name == "start":
            value = argument if isinstance(argument, dict) else {"start_index": int(argument or 0)}
            return self._start(int(value.get("start_index", 0)),
                               bool(value.get("run_only", False)))
        if name == "pause":
            return self._pause()
        if name == "resume":
            return self._resume()
        if name == "stop":
            return self._stop_run()
        if name == "status":
            return self.client.command("Status")
        if name == "retry":
            return self._retry()
        if name == "skip":
            return self._skip()
        if name == "device_event":
            return self._handle_event(argument)
        if name == "disconnected":
            return self._handle_disconnect(argument)
        raise SchedulerError(f"unknown scheduler action: {name}", "INVALID_ARGUMENT")

    def _start(self, start_index: int, run_only: bool = False) -> Dict[str, Any]:
        with self._lock:
            if self._state not in self.MUTABLE_STATES:
                raise SchedulerError("run is already active")
            if not self._tasks:
                raise SchedulerError("task list is empty", "EMPTY_TASK_LIST")
            if start_index >= len(self._tasks):
                raise SchedulerError("start_index is outside the task list", "INVALID_ARGUMENT")
            for index, task in enumerate(self._tasks):
                task.state = TaskState.QUEUED
                task.progress = 0
                task.remaining = task.count
                task.error = ""
                if index < start_index:
                    task.state = TaskState.SKIPPED
            self._current_index = start_index
            self._run_end_index = start_index if run_only else len(self._tasks) - 1
            self._encoder_angle = None
            self._stop_requested = False
            self._waiting_idle = False
            if self.history is not None:
                history_tasks = [task.to_dict(index)
                                 for index, task in enumerate(self._tasks)]
                self._history_id = self.history.begin(start_index, history_tasks)
        self._start_current()
        return self.snapshot()

    def _start_current(self) -> None:
        with self._lock:
            if (self._current_index is None or self._current_index >= len(self._tasks) or
                    (self._run_end_index is not None and
                     self._current_index > self._run_end_index)):
                self._state = RunState.COMPLETED
                self._message = "all tasks completed"
                self._current_index = None
                self._history_update("COMPLETED", True)
                return
            task = self._tasks[self._current_index]
            task.state = TaskState.STARTING
            self._state = RunState.STARTING
            self._message = f"starting VOR #{self._current_index + 1:02d}"
            parameters = (f"C={task.count} V={task.velocity:g} "
                          f"F={task.frequency:g} E={task.ex_mode}")
        try:
            self.client.command("Start", parameters)
            self._history_update("STARTING")
        except Exception as error:
            self._fault(str(error))
            raise

    def _pause(self) -> Dict[str, Any]:
        with self._lock:
            if self._state != RunState.RUNNING:
                raise SchedulerError("pause requires RUNNING")
        self.client.command("Pause")
        return self.snapshot()

    def _resume(self) -> Dict[str, Any]:
        with self._lock:
            if self._state != RunState.PAUSED:
                raise SchedulerError("resume requires PAUSED")
        self.client.command("Resume")
        return self.snapshot()

    def _stop_run(self) -> Dict[str, Any]:
        with self._lock:
            if self._state in self.MUTABLE_STATES:
                raise SchedulerError("no active run")
            self._stop_requested = True
            self._state = RunState.STOPPING
            self._message = "stopping task list"
        try:
            self.client.command("Stop")
        except CommandRejected as error:
            if error.ack.message != "NO_PC_SESSION":
                self._fault(str(error))
                raise
            self._finish_stopped()
        return self.snapshot()

    def _retry(self) -> Dict[str, Any]:
        with self._lock:
            if self._state != RunState.FAULT or self._current_index is None:
                raise SchedulerError("retry requires FAULT")
            task = self._tasks[self._current_index]
            task.error = ""
        try:
            ack = self.client.command("Status")
            with self._lock:
                self._state = RunState.PAUSED if ack.state == "PAUSED" else RunState.RUNNING
                task.state = TaskState.PAUSED if self._state == RunState.PAUSED else TaskState.RUNNING
                self._message = "reattached to active task"
                self._history_update(self._state.value)
        except CommandRejected as error:
            if error.ack.message != "NO_PC_SESSION":
                self._fault(str(error))
                raise
            self._start_current()
        return self.snapshot()

    def _skip(self) -> Dict[str, Any]:
        with self._lock:
            if self._state != RunState.FAULT or self._current_index is None:
                raise SchedulerError("skip requires FAULT")
        try:
            self.client.command("Stop")
            with self._lock:
                self._waiting_idle = True
                self._message = "stopping failed task before skip"
        except CommandRejected as error:
            if error.ack.message != "NO_PC_SESSION":
                raise
            self._advance(skipped=True)
        return self.snapshot()

    def _handle_event(self, event: Event) -> None:
        with self._lock:
            if self._current_index is None or self._current_index >= len(self._tasks):
                return
            task = self._tasks[self._current_index]
            task.progress = event.progress
            task.remaining = event.remaining
            if event.event == "ENCODER" and event.angle is not None:
                self._encoder_angle = event.angle
                return
            if event.event in {"RUNNING", "MOTOR_START", "PROGRESS"}:
                self._state = RunState.RUNNING
                task.state = TaskState.RUNNING
                self._message = "running"
            elif event.event == "PAUSED":
                self._state = RunState.PAUSED
                task.state = TaskState.PAUSED
                self._message = "paused"
                self._history_update("PAUSED")
            elif event.event == "COMPLETED":
                task.state = TaskState.COMPLETED
                task.progress = 100
                task.remaining = 0
                self._waiting_idle = True
                self._message = "waiting for device idle"
                self._history_update("RUNNING")
            elif event.event == "STOPPED":
                if self._stop_requested:
                    task.state = TaskState.STOPPED
                self._waiting_idle = True
            elif event.event == "ERROR" or event.code != 0:
                self._fault(f"device error {event.code}: {event.event}")
                return
            elif event.event == "IDLE":
                if self._stop_requested:
                    self._finish_stopped()
                elif self._waiting_idle:
                    self._advance(skipped=task.state == TaskState.ERROR)

    def _advance(self, skipped: bool = False) -> None:
        with self._lock:
            if self._current_index is None:
                return
            if skipped:
                self._tasks[self._current_index].state = TaskState.SKIPPED
                self._history_update("RUNNING")
            self._current_index += 1
            self._waiting_idle = False
        self._start_current()

    def _finish_stopped(self) -> None:
        with self._lock:
            self._state = RunState.IDLE
            self._message = "task list stopped"
            self._stop_requested = False
            self._waiting_idle = False
            self._current_index = None
            self._run_end_index = None
            self._history_update("STOPPED", True)

    def _fault(self, message: str) -> None:
        with self._lock:
            self._state = RunState.FAULT
            self._message = message
            if self._current_index is not None and self._current_index < len(self._tasks):
                task = self._tasks[self._current_index]
                task.state = TaskState.ERROR
                task.error = message
            self._history_update("FAULT", True)

    def _handle_disconnect(self, payload: Dict[str, Any]) -> None:
        with self._lock:
            if self._state not in self.MUTABLE_STATES:
                self._fault(payload.get("error") or "USB disconnected")
