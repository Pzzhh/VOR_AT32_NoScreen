# USB VOR Web Host

本目录是USB VOR网页上位机。首版仅支持VOR，由Python在内存中管理最多20个任务，并按顺序逐项发送到单片机。

The web interface is English-first. Tasks are automatically labelled `VOR #01`,
so users only enter motor parameters. Arrange mode, start-from selection, full-list
templates, and run history are managed by the Python host.

## Mac安装

```bash
cd tools/mac
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

依赖中包含Uvicorn所需的WebSocket协议实现。如果此前已经创建过虚拟环境，也需要重新执行上述安装命令。

编辑 `config.json`，明确填写USB串口。例如：

```json
"port": "/dev/cu.usbmodem1101"
```

程序不会自动扫描串口。端口为空时网页仍能启动，但会显示“USB未连接”。

### TCP模拟机

如果配置了有效的 `tcp.host` 和 `tcp.port`，程序优先使用TCP；否则使用串口：

```json
"tcp": {
  "host": "127.0.0.1",
  "port": 9000,
  "connect_timeout_seconds": 3.0
}
```

启动内置模拟设备：

```bash
.venv/bin/python tools/tcp_device_simulator.py --host 127.0.0.1 --port 9000
```

再启动网页服务并点击 Connect。模拟设备支持 Start、Status、Pause、Resume、Stop，使用与MCU相同的ASCII单行协议。要恢复真实USB串口，将 `tcp.host` 设为空字符串或将 `tcp.port` 设为0并重启网页服务。

### Linux TCP转TTY

Linux模拟程序已经连接到PTY/TTY peer时，可启动透明双向转发：

```bash
python3 tools/tcp_tty_bridge.py \
  --tty /tmp/ttyUSBpeer \
  --host 0.0.0.0 \
  --port 9000 \
  --baudrate 115200 \
  --verbose
```

Mac网页端将 `tcp.host` 设置为Linux机器IP、`tcp.port` 设置为9000。脚本不解析或修改数据，TCP断开后继续监听下一次连接；TTY暂时不存在时每秒重试。

启动：

```bash
./run.sh
```

浏览器打开 `http://127.0.0.1:8765`。

## Templates and history

- Arrange is drag-only and available while the queue is idle or completed.
- Right-click a task to run only that task or start the queue from that task;
  earlier tasks are marked skipped.
- Up to 5 complete-list templates are stored in `data/templates.json`.
- Templates can also be downloaded and uploaded as JSON files.
- The latest 20 runs are stored in `data/history.json`; active records are updated
  at task boundaries and become `INTERRUPTED` after an unclean service restart.
- Both files are written atomically. Invalid files are renamed with a
  `.corrupt-<timestamp>` suffix and reported in `/api/status`.

The location and limits can be configured with `data_dir`, `template_limit`, and
`history_limit` in `config.json`.

也可以指定另一份配置：

```bash
VOR_HOST_CONFIG=/absolute/path/config.json .venv/bin/python -m app
```

## Linux迁移

复制 `tools/mac` 源码，在Linux重新创建 `.venv`并安装同一份依赖，只需把 `config.json`中的端口改为 `/dev/ttyACM0`或socat生成的PTY。

## REST API

- `GET /api/status`、`GET /api/config`、`GET /api/logs`
- `POST /api/serial/connect`、`disconnect`
- `GET/POST /api/tasks`
- `PUT/DELETE /api/tasks/{id}`
- `POST /api/tasks/reorder`
- `GET/POST /api/templates`
- `DELETE /api/templates/{id}`
- `POST /api/templates/{id}/load`、`GET /api/templates/{id}/download`
- `POST /api/templates/import`
- `GET /api/history`、`GET/DELETE /api/history/{id}`、`DELETE /api/history`
- `POST /api/run/start`、`pause`、`resume`、`stop`、`status`、`retry`、`skip`
- `WebSocket /ws/events`

OpenAPI页面：`http://127.0.0.1:8765/docs`。

## 测试

```bash
cd tools/mac
.venv/bin/python -m pytest -q
```
