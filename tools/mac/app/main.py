from __future__ import annotations

import asyncio
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Any, Dict, List

from fastapi import FastAPI, HTTPException, Query, WebSocket, WebSocketDisconnect
from fastapi.exceptions import RequestValidationError
from fastapi.responses import FileResponse, JSONResponse, Response
from fastapi.staticfiles import StaticFiles

from .config import AppConfig
from .models import (ReorderInput, StartRunInput, TemplateCreateInput,
                     TemplateImportInput, VorTaskInput, api_result)
from .scheduler import SchedulerError, TaskScheduler
from .serial_client import SerialClient, SerialClientError
from .storage import HistoryStore, TemplateStore


class HostService:
    def __init__(self, config: AppConfig) -> None:
        self.config = config
        data_dir = Path(config.data_dir)
        self.templates = TemplateStore(data_dir / "templates.json",
                                       config.template_limit)
        self.history = HistoryStore(data_dir / "history.json",
                                    config.history_limit)
        self.serial = SerialClient(config.serial, config.log_limit, config.tcp)
        self.scheduler = TaskScheduler(self.serial, self.history)

    def close(self) -> None:
        self.scheduler.close()
        self.serial.close()

    def snapshot(self) -> Dict[str, Any]:
        return {
            "serial": self.serial.snapshot(),
            "run": self.scheduler.snapshot(),
            "storage": {"template_count": len(self.templates.list()),
                        "history_count": len(self.history.list()),
                        "errors": [error for error in
                                   (self.templates.error, self.history.error) if error]},
        }


def create_app(config: AppConfig) -> FastAPI:
    service = HostService(config)

    @asynccontextmanager
    async def lifespan(_app: FastAPI):
        try:
            yield
        finally:
            service.close()

    app = FastAPI(title="USB VOR Host", version="0.1.0", lifespan=lifespan)
    app.state.service = service
    static_dir = Path(__file__).resolve().parent / "static"
    app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")

    @app.exception_handler(RequestValidationError)
    async def validation_error(_request: Any, error: RequestValidationError) -> JSONResponse:
        return JSONResponse(status_code=422,
                            content=api_result(None, "INVALID_ARGUMENT", str(error)))

    @app.exception_handler(SchedulerError)
    async def scheduler_error(_request: Any, error: SchedulerError) -> JSONResponse:
        status = 404 if error.code == "NOT_FOUND" else 409
        if error.code in {"INVALID_ARGUMENT", "EMPTY_TASK_LIST"}:
            status = 400
        return JSONResponse(status_code=status,
                            content=api_result(None, error.code, str(error)))

    @app.exception_handler(SerialClientError)
    async def serial_error(_request: Any, error: SerialClientError) -> JSONResponse:
        return JSONResponse(status_code=503,
                            content=api_result(None, "SERIAL_ERROR", str(error)))

    @app.get("/", include_in_schema=False)
    async def index() -> FileResponse:
        return FileResponse(static_dir / "index.html")

    @app.get("/api/status")
    async def status() -> Dict[str, Any]:
        return api_result(service.snapshot())

    @app.get("/api/config")
    async def public_config() -> Dict[str, Any]:
        return api_result({
            "serial": {
                "port": config.serial.port,
                "baudrate": config.serial.baudrate,
                "ack_timeout_seconds": config.serial.ack_timeout_seconds,
                "run_timeout_seconds": config.serial.run_timeout_seconds,
                "reconnect_interval_seconds": config.serial.reconnect_interval_seconds,
            },
            "tcp": {
                "enabled": config.tcp.enabled,
                "host": config.tcp.host,
                "port": config.tcp.port,
                "connect_timeout_seconds": config.tcp.connect_timeout_seconds,
            },
            "web": {"host": config.web.host, "port": config.web.port},
            "task_limit": TaskScheduler.MAX_TASKS,
            "template_limit": config.template_limit,
            "history_limit": config.history_limit,
        })

    @app.post("/api/serial/connect")
    async def serial_connect() -> Dict[str, Any]:
        service.serial.connect()
        return api_result(service.serial.snapshot())

    @app.post("/api/serial/disconnect")
    async def serial_disconnect() -> Dict[str, Any]:
        service.serial.disconnect()
        return api_result(service.serial.snapshot())

    @app.get("/api/tasks")
    async def tasks() -> Dict[str, Any]:
        return api_result(service.scheduler.tasks())

    @app.post("/api/tasks", status_code=201)
    async def add_task(data: VorTaskInput) -> Dict[str, Any]:
        return api_result(service.scheduler.add_task(data))

    @app.put("/api/tasks/{task_id}")
    async def update_task(task_id: int, data: VorTaskInput) -> Dict[str, Any]:
        return api_result(service.scheduler.update_task(task_id, data))

    @app.delete("/api/tasks/{task_id}")
    async def delete_task(task_id: int) -> Dict[str, Any]:
        service.scheduler.delete_task(task_id)
        return api_result()

    @app.post("/api/tasks/reorder")
    async def reorder(data: ReorderInput) -> Dict[str, Any]:
        service.scheduler.reorder(data.ids)
        return api_result(service.scheduler.tasks())

    @app.get("/api/templates")
    async def templates() -> Dict[str, Any]:
        return api_result(service.templates.list())

    @app.post("/api/templates", status_code=201)
    async def create_template(data: TemplateCreateInput) -> Dict[str, Any]:
        tasks = service.scheduler.template_tasks()
        if not tasks:
            raise SchedulerError("task list is empty", "EMPTY_TASK_LIST")
        try:
            return api_result(service.templates.create(data.name, tasks))
        except OverflowError as error:
            raise SchedulerError(str(error), "TEMPLATE_LIMIT")

    @app.delete("/api/templates/{template_id}")
    async def delete_template(template_id: str) -> Dict[str, Any]:
        if not service.scheduler.is_mutable():
            raise SchedulerError("templates are locked while running", "RUN_ACTIVE")
        try:
            service.templates.delete(template_id)
        except KeyError:
            raise SchedulerError("template not found", "NOT_FOUND")
        return api_result()

    def load_template_tasks(tasks: Any) -> List[Dict[str, Any]]:
        validated = [VorTaskInput(**task) for task in tasks]
        return service.scheduler.replace_tasks(validated)

    @app.post("/api/templates/{template_id}/load")
    async def load_template(template_id: str) -> Dict[str, Any]:
        try:
            template = service.templates.get(template_id)
        except KeyError:
            raise SchedulerError("template not found", "NOT_FOUND")
        return api_result(load_template_tasks(template["tasks"]))

    @app.get("/api/templates/{template_id}/download")
    async def download_template(template_id: str) -> Response:
        try:
            template = service.templates.get(template_id)
        except KeyError:
            raise SchedulerError("template not found", "NOT_FOUND")
        import json
        payload = json.dumps(template, ensure_ascii=False, indent=2)
        return Response(payload, media_type="application/json",
                        headers={"Content-Disposition":
                                 f'attachment; filename="vor-template-{template_id[:8]}.json"'})

    @app.post("/api/templates/import")
    async def import_template(data: TemplateImportInput) -> Dict[str, Any]:
        return api_result(service.scheduler.replace_tasks(data.tasks))

    @app.get("/api/history")
    async def history() -> Dict[str, Any]:
        return api_result(service.history.list())

    @app.get("/api/history/{history_id}")
    async def history_item(history_id: str) -> Dict[str, Any]:
        try:
            return api_result(service.history.get(history_id))
        except KeyError:
            raise SchedulerError("history not found", "NOT_FOUND")

    @app.delete("/api/history/{history_id}")
    async def delete_history(history_id: str) -> Dict[str, Any]:
        try:
            service.history.delete(history_id)
        except KeyError:
            raise SchedulerError("history not found", "NOT_FOUND")
        return api_result()

    @app.delete("/api/history")
    async def clear_history() -> Dict[str, Any]:
        service.history.clear()
        return api_result()

    async def run_action(name: str, argument: Any = None) -> Dict[str, Any]:
        loop = asyncio.get_running_loop()
        result = await loop.run_in_executor(None, service.scheduler.execute,
                                            name, argument, 15.0)
        return api_result(result)

    @app.post("/api/run/start")
    async def start(data: StartRunInput) -> Dict[str, Any]:
        return await run_action("start", {"start_index": data.start_index,
                                          "run_only": data.run_only})

    @app.post("/api/run/pause")
    async def pause() -> Dict[str, Any]:
        return await run_action("pause")

    @app.post("/api/run/resume")
    async def resume() -> Dict[str, Any]:
        return await run_action("resume")

    @app.post("/api/run/stop")
    async def stop() -> Dict[str, Any]:
        return await run_action("stop")

    @app.post("/api/run/status")
    async def run_status() -> Dict[str, Any]:
        await run_action("status")
        return api_result(service.snapshot())

    @app.post("/api/run/retry")
    async def retry() -> Dict[str, Any]:
        return await run_action("retry")

    @app.post("/api/run/skip")
    async def skip() -> Dict[str, Any]:
        return await run_action("skip")

    @app.get("/api/logs")
    async def logs(offset: int = Query(0, ge=0),
                   limit: int = Query(200, ge=1, le=500)) -> Dict[str, Any]:
        return api_result(service.serial.logs(offset, limit))

    @app.websocket("/ws/events")
    async def events(websocket: WebSocket) -> None:
        await websocket.accept()
        sequence = 0
        try:
            while True:
                logs = service.serial.logs_after(sequence)
                if logs:
                    sequence = logs[-1]["sequence"]
                await websocket.send_json({
                    "type": "snapshot",
                    "data": service.snapshot(),
                    "logs": logs,
                })
                await asyncio.sleep(0.25)
        except WebSocketDisconnect:
            return

    return app
