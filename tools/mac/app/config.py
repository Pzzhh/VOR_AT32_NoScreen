from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass(frozen=True)
class SerialConfig:
    port: str
    baudrate: int = 115200
    ack_timeout_seconds: float = 5.0
    run_timeout_seconds: float = 120.0
    reconnect_interval_seconds: float = 2.0


@dataclass(frozen=True)
class WebConfig:
    host: str = "127.0.0.1"
    port: int = 8765


@dataclass(frozen=True)
class AppConfig:
    serial: SerialConfig
    web: WebConfig
    log_limit: int = 1000
    data_dir: str = "data"
    template_limit: int = 5
    history_limit: int = 20
    telemetry: Optional[SerialConfig] = None


def default_config_path() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent / "config.json"
    return Path(__file__).resolve().parents[1] / "config.json"


def load_config(path: Optional[Path] = None) -> AppConfig:
    target = path or default_config_path()
    with target.open("r", encoding="utf-8") as stream:
        raw = json.load(stream)
    data_dir = str(raw.get("data_dir", "data"))
    if not Path(data_dir).is_absolute():
        data_dir = str((target.parent / data_dir).resolve())
    config = AppConfig(
        serial=SerialConfig(**raw.get("serial", {})),
        web=WebConfig(**raw.get("web", {})),
        log_limit=int(raw.get("log_limit", 1000)),
        data_dir=data_dir,
        template_limit=int(raw.get("template_limit", 5)),
        history_limit=int(raw.get("history_limit", 20)),
        telemetry=SerialConfig(**raw.get("telemetry", {"port": ""})),
    )
    if config.serial.baudrate <= 0:
        raise ValueError("serial.baudrate must be positive")
    if config.log_limit < 100:
        raise ValueError("log_limit must be at least 100")
    if config.template_limit < 1 or config.history_limit < 1:
        raise ValueError("storage limits must be positive")
    return config
