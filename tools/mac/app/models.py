from __future__ import annotations

from dataclasses import asdict, dataclass
from enum import Enum
from typing import Any, Optional

from pydantic import BaseModel, ConfigDict, Field, model_validator


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
    model_config = ConfigDict(extra="forbid")
    mode: str = "VOR"
    count: Optional[int] = Field(default=None, ge=1, le=1000)
    velocity: Optional[float] = Field(default=None, ge=-360, le=360)
    frequency: Optional[float] = Field(default=None, gt=0, le=11)
    ex_mode: Optional[int] = Field(default=None, ge=0, le=3)
    seconds: Optional[int] = Field(default=None, ge=1, le=1000)
    incline_degree: Optional[float] = Field(default=None, ge=0, le=50)

    @model_validator(mode="after")
    def validate_mode_fields(self) -> "VorTaskInput":
        self.mode = self.mode.upper()
        required = ({"VOR": ("count", "velocity", "frequency", "ex_mode"),
                     "OVAR": ("seconds", "velocity", "incline_degree")}
                    .get(self.mode))
        if required is None:
            raise ValueError("mode must be VOR or OVAR")
        missing = [field for field in required if getattr(self, field) is None]
        if missing:
            raise ValueError(f"missing fields for {self.mode}: {', '.join(missing)}")
        return self


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


class SerialPortInput(BaseModel):
    port: str = Field(min_length=1, max_length=256)


@dataclass
class VorTask:
    id: int
    mode: str = "VOR"
    count: Optional[int] = None
    velocity: Optional[float] = None
    frequency: Optional[float] = None
    ex_mode: Optional[int] = None
    seconds: Optional[int] = None
    incline_degree: Optional[float] = None
    state: TaskState = TaskState.QUEUED
    progress: int = 0
    remaining: int = 0
    error: str = ""

    @classmethod
    def from_input(cls, task_id: int, data: VorTaskInput) -> "VorTask":
        remaining = data.count if data.mode == "VOR" else data.seconds
        return cls(id=task_id, remaining=remaining or 0, **data.model_dump())

    def update_from(self, data: VorTaskInput) -> None:
        self.mode = data.mode
        self.count = data.count
        self.velocity = data.velocity
        self.frequency = data.frequency
        self.ex_mode = data.ex_mode
        self.seconds = data.seconds
        self.incline_degree = data.incline_degree
        self.remaining = (data.count if data.mode == "VOR" else data.seconds) or 0
        self.progress = 0
        self.error = ""
        self.state = TaskState.QUEUED

    def to_dict(self, position: int = 0) -> dict[str, Any]:
        result = asdict(self)
        result["state"] = self.state.value
        result["display_name"] = f"{self.mode} #{position + 1:02d}"
        return result

    def parameters(self) -> str:
        if self.mode == "VOR":
            return f"C={self.count} V={self.velocity:g} F={self.frequency:g} E={self.ex_mode}"
        return f"S={self.seconds} V={self.velocity:g}"


def api_result(data: Any = None, code: str = "OK", message: str = "") -> dict[str, Any]:
    return {"ok": code == "OK", "code": code, "message": message, "data": data}
