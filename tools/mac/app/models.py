from __future__ import annotations

from dataclasses import asdict, dataclass
from enum import Enum
from typing import Any

from pydantic import BaseModel, Field


class TaskState(str, Enum):
    QUEUED = "QUEUED"
    STARTING = "STARTING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    COMPLETED = "COMPLETED"
    ERROR = "ERROR"
    SKIPPED = "SKIPPED"
    STOPPED = "STOPPED"


class RunState(str, Enum):
    IDLE = "IDLE"
    STARTING = "STARTING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    STOPPING = "STOPPING"
    FAULT = "FAULT"
    COMPLETED = "COMPLETED"


class VorTaskInput(BaseModel):
    count: int = Field(ge=1, le=1000)
    velocity: float = Field(ge=-360, le=360)
    frequency: float = Field(gt=0, le=11)
    ex_mode: int = Field(ge=0, le=3)


class StartRunInput(BaseModel):
    start_index: int = Field(default=0, ge=0, le=19)
    run_only: bool = False


class ReorderInput(BaseModel):
    ids: list[int]


class TemplateCreateInput(BaseModel):
    name: str = Field(min_length=1, max_length=60)


class TemplateImportInput(BaseModel):
    name: str = Field(default="Imported template", min_length=1, max_length=60)
    tasks: list[VorTaskInput] = Field(min_length=1, max_length=20)


@dataclass
class VorTask:
    id: int
    count: int
    velocity: float
    frequency: float
    ex_mode: int
    state: TaskState = TaskState.QUEUED
    progress: int = 0
    remaining: int = 0
    error: str = ""

    @classmethod
    def from_input(cls, task_id: int, data: VorTaskInput) -> "VorTask":
        return cls(id=task_id, remaining=data.count, **data.model_dump())

    def update_from(self, data: VorTaskInput) -> None:
        self.count = data.count
        self.velocity = data.velocity
        self.frequency = data.frequency
        self.ex_mode = data.ex_mode
        self.remaining = data.count
        self.progress = 0
        self.error = ""
        self.state = TaskState.QUEUED

    def to_dict(self, position: int = 0) -> dict[str, Any]:
        result = asdict(self)
        result["state"] = self.state.value
        result["display_name"] = f"VOR #{position + 1:02d}"
        return result


def api_result(data: Any = None, code: str = "OK", message: str = "") -> dict[str, Any]:
    return {"ok": code == "OK", "code": code, "message": message, "data": data}
