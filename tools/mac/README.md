# USB VOR / OVAR Web Host

本目录是 USB VOR/OVAR 网页上位机。Python 在内存中管理最多20个任务，并按顺序逐项发送到单片机。

The web interface is English-first. Tasks are automatically labelled `VOR #01` or `OVAR #01`,
so users only enter motor parameters. Arrange mode, start-from selection, full-list
templates, and run history are managed by the Python host.

## OVAR command split

An OVAR item remains one task in the web list, but the host executes three independent
device commands. Incline movement and OVAR rotation are never combined in one command:

```text
Ctrl:INC.Start ID=n A=15
Ctrl:OVAR.Start ID=n S=60 V=30
Ctrl:INC.Start ID=n A=0
```

The host waits for `COMPLETED` and `IDLE` after each step before starting the next.
`Ctrl:OVAR.Start` accepts only `S` (seconds) and `V` (velocity); `Ctrl:INC.Start`
accepts only `A` (target angle, 0..50 degrees). Stop or an OVAR execution error also
triggers a separate `INC.Start A=0` recovery step before the queue ends.

## Mac安装

```bash
cd tools/mac
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

依赖中包含Uvicorn所需的WebSocket协议实现。如果此前已经创建过虚拟环境，也需要重新执行上述安装命令。

网页顶部会自动列出可用串口；在下拉列表中选择设备后点击 **Connect**。Windows 会显示为 `COM3`、`COM7` 等，无需手工修改配置。若端口刚插入，点击列表旁的刷新按钮。

也可在 `config.json` 中预设 USB 串口，例如：

```json
"port": "/dev/cu.usbmodem1101"
```

程序不会自动扫描串口。端口为空时网页仍能启动，但会显示“USB未连接”。

推荐使用本机启动脚本。Windows 在 PowerShell 或命令提示符中执行：

```powershell
py start_local.py
```

macOS / Linux 执行：

```bash
python3 start_local.py
```

脚本首次运行会自动创建 `.venv`、安装依赖、启动服务并打开浏览器。以后直接执行同一命令即可；按 `Ctrl+C` 停止。若不希望自动打开浏览器，追加 `--no-browser`。

原有 macOS 启动方式仍可使用：

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

## Windows 打包

Windows 10/11 安装 Python 3.10 或更高版本后，双击或在命令提示符执行 `build_windows.bat`。脚本会创建虚拟环境、安装依赖并生成：

`dist\VOR-Control\VOR-Control.exe`

将整个 `dist\VOR-Control` 目录复制到目标电脑后运行其中的 `VOR-Control.exe`，再用浏览器打开 `http://127.0.0.1:8765`。请保留同目录的 `config.json`，可按需设置波特率。

## REST API

- `GET /api/status`、`GET /api/config`、`GET /api/logs`
- `GET /api/serial/ports`、`PUT /api/serial/port`、`POST /api/serial/connect`、`disconnect`
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
# Dual-CDC telemetry

Select the command CDC port in **Command port** and the second CDC port in
**Telemetry port** before connecting. During a motor task the page plots the
latest ten seconds of encoder position and calculated speed at 60 samples/s.
Each CDC2 packet is 24 bytes: the fixed header `AA 55`, a little-endian 16-bit
packet sequence, followed by five `(position x10, speed x10)` signed 16-bit
sample pairs. The receiver scans for the header to recover packet alignment.

CDC2 is optional. To run with a single virtual serial port, leave **Telemetry
port** set to **No CDC2 (optional)**. Commands and motor tasks continue to work;
only the live encoder chart is unavailable. An existing CDC2 selection can be
cleared by choosing that option before connecting.

Use **Save encoder CSV** to download buffered telemetry with local timestamps,
sample and packet sequences, position in degrees, and speed in degrees per
second. The in-memory buffer retains up to ten minutes (36,000 samples at 60 Hz).
