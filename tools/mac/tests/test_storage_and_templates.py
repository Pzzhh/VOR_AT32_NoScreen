from pathlib import Path

from fastapi.testclient import TestClient

from app.config import AppConfig, SerialConfig, WebConfig
from app.main import create_app
from app.storage import HistoryStore, TemplateStore


TASK = {"count": 20, "velocity": 50, "frequency": 1.6, "ex_mode": 0}


def test_template_store_limit_and_reload(tmp_path: Path) -> None:
    store = TemplateStore(tmp_path / "templates.json", 2)
    first = store.create("one", [TASK])
    store.create("two", [TASK])
    try:
        store.create("three", [TASK])
        assert False, "expected template limit"
    except OverflowError:
        pass
    assert TemplateStore(tmp_path / "templates.json", 2).get(first["id"])["name"] == "one"


def test_active_history_becomes_interrupted(tmp_path: Path) -> None:
    path = tmp_path / "history.json"
    store = HistoryStore(path, 20)
    item_id = store.begin(0, [TASK])
    assert HistoryStore(path, 20).get(item_id)["result"] == "INTERRUPTED"


def test_history_keeps_latest_twenty(tmp_path: Path) -> None:
    store = HistoryStore(tmp_path / "history.json", 20)
    first = store.begin(0, [TASK])
    for _ in range(20):
        store.begin(0, [TASK])
    assert len(store.list()) == 20
    try:
        store.get(first)
        assert False, "oldest history should be removed"
    except KeyError:
        pass


def test_template_api_replaces_tasks_and_automatic_names(tmp_path: Path) -> None:
    config = AppConfig(SerialConfig(port=""), WebConfig(), 200,
                       data_dir=str(tmp_path))
    with TestClient(create_app(config)) as client:
        assert client.post("/api/tasks", json=TASK).status_code == 201
        created = client.post("/api/templates", json={"name": "standard"})
        assert created.status_code == 201
        template_id = created.json()["data"]["id"]
        task_id = client.get("/api/tasks").json()["data"][0]["id"]
        client.delete(f"/api/tasks/{task_id}")
        assert client.post(f"/api/templates/{template_id}/load").status_code == 200
        task = client.get("/api/tasks").json()["data"][0]
        assert task["display_name"] == "VOR #01"
        assert "name" not in task
