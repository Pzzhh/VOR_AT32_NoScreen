from __future__ import annotations

import json
import os
import threading
import uuid
from copy import deepcopy
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List


def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


class JsonListStore:
    def __init__(self, path: Path, limit: int) -> None:
        self.path = path
        self.limit = limit
        self._lock = threading.RLock()
        self.error = ""
        self._items: List[Dict[str, Any]] = []
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._load()

    def _load(self) -> None:
        if not self.path.exists():
            return
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
            if not isinstance(data, list):
                raise ValueError("root must be a JSON array")
            self._items = data[-self.limit:]
        except Exception as error:
            self.error = f"{self.path.name}: {error}"
            stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
            backup = self.path.with_name(f"{self.path.name}.corrupt-{stamp}")
            try:
                os.replace(str(self.path), str(backup))
            except OSError:
                pass

    def _save(self) -> None:
        temporary = self.path.with_suffix(self.path.suffix + ".tmp")
        with temporary.open("w", encoding="utf-8") as stream:
            json.dump(self._items, stream, ensure_ascii=False, indent=2)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(str(temporary), str(self.path))

    def list(self) -> List[Dict[str, Any]]:
        with self._lock:
            return deepcopy(list(reversed(self._items)))

    def get(self, item_id: str) -> Dict[str, Any]:
        with self._lock:
            for item in self._items:
                if item.get("id") == item_id:
                    return deepcopy(item)
        raise KeyError(item_id)

    def delete(self, item_id: str) -> None:
        with self._lock:
            before = len(self._items)
            self._items = [item for item in self._items if item.get("id") != item_id]
            if len(self._items) == before:
                raise KeyError(item_id)
            self._save()

    def clear(self) -> None:
        with self._lock:
            self._items = []
            self._save()


class TemplateStore(JsonListStore):
    def create(self, name: str, tasks: List[Dict[str, Any]]) -> Dict[str, Any]:
        with self._lock:
            if len(self._items) >= self.limit:
                raise OverflowError("template library is full")
            item = {"id": uuid.uuid4().hex, "name": name,
                    "created_at": now_iso(), "tasks": deepcopy(tasks)}
            self._items.append(item)
            self._save()
            return deepcopy(item)


class HistoryStore(JsonListStore):
    ACTIVE_RESULTS = {"STARTING", "RUNNING", "PAUSED", "STOPPING"}

    def __init__(self, path: Path, limit: int) -> None:
        super().__init__(path, limit)
        changed = False
        for item in self._items:
            if item.get("result") in self.ACTIVE_RESULTS:
                item["result"] = "INTERRUPTED"
                item["ended_at"] = now_iso()
                changed = True
        if changed:
            self._save()

    def begin(self, start_index: int, tasks: List[Dict[str, Any]]) -> str:
        with self._lock:
            item_id = uuid.uuid4().hex
            item = {"id": item_id, "started_at": now_iso(), "ended_at": None,
                    "start_index": start_index, "result": "STARTING",
                    "tasks": deepcopy(tasks)}
            self._items.append(item)
            self._items = self._items[-self.limit:]
            self._save()
            return item_id

    def update(self, item_id: str, result: str, tasks: List[Dict[str, Any]],
               terminal: bool = False) -> None:
        with self._lock:
            for item in self._items:
                if item.get("id") == item_id:
                    item["result"] = result
                    item["tasks"] = deepcopy(tasks)
                    if terminal:
                        item["ended_at"] = now_iso()
                    else:
                        item["ended_at"] = None
                    self._save()
                    return
