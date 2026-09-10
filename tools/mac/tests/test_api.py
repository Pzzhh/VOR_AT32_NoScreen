from pathlib import Path
from unittest.mock import patch

from fastapi.testclient import TestClient

from app.config import AppConfig, SerialConfig, WebConfig
from app.main import create_app


def make_client(data_dir="/tmp/vor-web-test") -> TestClient:
    config = AppConfig(SerialConfig(port=""), WebConfig(), 200,
                       data_dir=data_dir)
    return TestClient(create_app(config))


def test_task_crud_and_status() -> None:
    with make_client() as client:
        payload = {"count": 20, "velocity": 50,
                   "frequency": 1.6, "ex_mode": 0}
        created = client.post("/api/tasks", json=payload)
        assert created.status_code == 201
        task_id = created.json()["data"]["id"]
        assert client.get("/api/tasks").json()["data"][0]["id"] == task_id
        assert client.get("/api/status").json()["data"]["run"]["task_count"] == 1
        assert client.delete(f"/api/tasks/{task_id}").status_code == 200


def test_validation_uses_common_envelope() -> None:
    with make_client() as client:
        response = client.post("/api/tasks", json={"count": 0,
                               "velocity": 50, "frequency": 1.6, "ex_mode": 0})
        assert response.status_code == 422
        assert response.json()["ok"] is False
        assert response.json()["code"] == "INVALID_ARGUMENT"


def test_websocket_delivers_snapshot() -> None:
    with make_client() as client:
        with client.websocket_connect("/ws/events") as websocket:
            message = websocket.receive_json()
            assert message["type"] == "snapshot"
            assert message["data"]["run"]["state"] == "IDLE"
            assert message["data"]["serial"]["connected"] is False


def test_serial_port_can_be_listed_and_selected() -> None:
    with make_client() as client:
        with patch("app.serial_client.list_ports.comports", return_value=[]):
            assert client.get("/api/serial/ports").json()["data"] == []
        selected = client.put("/api/serial/port", json={"port": "COM7"})
        assert selected.status_code == 200
        assert selected.json()["data"]["port"] == "COM7"
