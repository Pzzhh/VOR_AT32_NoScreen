# 串口职责分配与协议规划

## 1. 文档目的

本文基于当前代码梳理 RK 左串口、RK 右串口和 USB ACM 串口的既有职责，并规划后续通信边界。

- RK 左、右串口：保留原有 RK3588 数据处理，并在运动执行期间返回电机实时速度。
- USB ACM：集中承担设备控制、任务管理、状态查询和调试日志。
- Windows、Linux 模拟器和 AT32 使用相同上层语义，仅替换底层串口实现。

本文是实施设计，不表示新增协议已经完成。

## 2. 当前代码中的通信分工

### 2.1 RK 右串口

| 平台 | 底层端口 |
| --- | --- |
| AT32 | USART1，115200 |
| Windows 模拟器 | 原 `SerialPort_Init(..., L"COM1")` |
| Linux 模拟器 | `/tmp/ttyRKRight` |

当前发送内容：

- `filename:<任务描述>\r\n`：下发当前任务的时间、名称、模式和参数。
- `recordingEnd`：当前任务录像结束。
- `requiredstatus`：查询 RK 状态。
- `pause`、`cancel`、`interrupt`、`clear`、`UART Test`。
- `datetime:YYYY-MM-DDTHH:MM:SS`：响应 RK 的时间请求。

当前接收并处理：

- `ok`：命令确认。
- `ready`：RK 启动完成。
- `status:<code>`：错误或运行状态。
- `wifiname:<name>`：Wi-Fi 名称。
- `reqtime`：请求当前时间。
- `DFU`、`FACTORY Reset`、`UART Test`。

### 2.2 RK 左串口

| 平台 | 底层端口 |
| --- | --- |
| AT32 | USART6，115200 |
| Windows 模拟器 | 当前未建立独立的第二个 COM 连接 |
| Linux 模拟器 | `/tmp/ttyRKLeft` |

上层协议与 RK 右串口对称。`LR=0` 选择左通道，`LR=1` 选择右通道；左右分别保存确认状态、错误码、Wi-Fi 名称和接收缓冲区。

当前任务控制会同时设置左右通道的 `SendTaskArray`、`RecordStop`、`ReqRKErrorStatus`、`Pause` 或 `Cancel` 状态，因此两个 RK 是并行协作设备，而不是主备关系。

### 2.3 USB ACM

| 平台 | 底层端口 |
| --- | --- |
| AT32 | USB CDC ACM，通道 1 |
| Linux 模拟器 | `/tmp/ttyUSB` |

当前能力：

- `HAL_USB_ACM_Init/Send/Read/IsReady` 提供字节流接口。
- 以换行作为命令结束符，支持退格处理。
- AT32 已有 `help`、`lvmem`、`heap`、`stack`、`pri`、`tasks`、`alltask`、`memcheck` 等诊断命令。
- 任务新增、删除、保存、移动和错误状态会通过 USB 输出日志。
- USB 尚未形成稳定的开始、暂停、继续、停止和任务配置控制协议。

## 3. 目标职责分配

### 3.1 RK 左、右：实时协作与运动数据

保留所有现有命令和应答，不把 USB 控制命令混入 RK 通道。

新增电机实际速度反馈：

- 数据源：C610 当前反馈 `motor_chassis[0].speed_rpm`，不是目标速度 `C610_Set_Speed`。
- 发布条件：`IsRunning=1` 且控制状态为实际运动阶段；准备、任务间隔、暂停、回零和停止阶段不连续发布。
- 发布频率：默认 20 Hz（50 ms 一帧），不跟随高速 CAN/PID 循环逐帧发送，避免阻塞控制周期。
- 分配方式：同一实际速度同时广播给 RK 左、右，使两端录像/数据记录使用同一时间基准。
- 停止行为：退出运动阶段时向两路各发送一帧零速度或带停止状态的最终帧。

建议帧格式：

```text
motor_speed:rpm=<signed_rpm>,tick=<milliseconds>,task=<index>,state=<run|pause|stop>\n
```

示例：

```text
motor_speed:rpm=-1260,tick=183450,task=2,state=run
```

新增帧使用独立前缀 `motor_speed:`，不会与当前 `ok`、`ready`、`status:`、`wifiname:` 和文件名协议冲突。

### 3.2 USB ACM：控制、配置和诊断

USB 作为唯一的上位机控制入口。协议继续采用 ASCII 单行文本，以 `\n` 或 `\r\n` 分帧。

第一阶段命令集合：

| 命令 | 作用 | 建议响应 |
| --- | --- | --- |
| `ctrl status` | 查询设备、任务、暂停和错误状态 | `ok status ...` |
| `ctrl start` | 从第一个任务开始 | `ok start` 或 `err busy` |
| `ctrl start <index>` | 从指定任务开始 | `ok start <index>` |
| `ctrl pause` | 暂停当前运动 | `ok pause` |
| `ctrl resume` | 继续当前运动 | `ok resume` |
| `ctrl stop` | 安全停止当前运动 | `ok stop` |
| `task list` | 返回任务摘要 | 多行 `data task ...`，最后 `ok list` |
| `task get <index>` | 返回完整任务参数 | `data task ...` |
| `task set <index> ...` | 新增或更新任务 | `ok task <index>` |
| `task delete <index>` | 删除任务 | `ok delete <index>` |
| `task save` | 写入现有持久化接口 | `ok save` |

原有诊断命令保留，但应与控制命令共享同一个分发器。所有命令必须产生一个明确的最终 `ok` 或 `err` 响应；日志使用 `log ` 前缀，避免上位机把异步日志当成命令响应。

USB 控制函数不直接操作电机：解析器只调用已有的任务管理、消息中心和 `Task_control_Req*` 接口，由原控制线程执行状态转换和安全检查。

## 4. 数据流与并发边界

```text
USB 上位机
  -> USB 行解析器
  -> 控制/任务命令分发
  -> Message Center / Task Control
  -> 现有运动控制状态机

现有运动控制状态机
  -> RK 左、右原有任务/录像命令
  -> C610 实际速度快照
  -> 20 Hz RK 左、右速度广播
```

- USB 接收处理不得在 USB 中断中执行任务修改或运动控制。
- RK 速度广播读取速度快照，不得阻塞 PID/CAN 更新。
- 三个串口分别维护发送锁或发送队列，避免日志、命令和速度帧交叉写入。
- RK 原有逐字节换行接收缓冲区只有 64 字节；新帧和后续协议应限制单行长度，或在实施时扩大并增加溢出丢帧计数。
- USB 控制命令涉及任务数组时必须使用现有任务管理同步机制，禁止解析器直接改写运行中的任务内存。

## 5. 平台映射

| 逻辑通道 | AT32 | Linux | Windows目标状态 |
| --- | --- | --- | --- |
| RK 右 | USART1 | `/tmp/ttyRKRight` | 独立可配置 COM 端口 |
| RK 左 | USART6 | `/tmp/ttyRKLeft` | 新增独立可配置 COM 端口 |
| USB 控制 | USB CDC ACM CH1 | `/tmp/ttyUSB` | 可选独立 COM/命名管道后端 |

- 保持所有现有 `use_windows` 判断不变。
- Linux 差异只放在 `use_linux` 分支。
- 上层命令解析、状态机和速度帧格式不得包含平台判断。

## 6. 分阶段实施计划

1. 抽象三路发送入口和端口级发送锁，保持 RK 现有协议行为不变。
2. 增加只读的电机速度快照接口和 20 Hz 发布器，同时广播到两个 RK。
3. 将 USB 现有诊断命令整理为统一命令表，加入运行控制和状态查询。
4. 通过现有任务管理接口加入任务读取、修改、删除和持久化命令。
5. 为 Windows 增加 RK 左和 USB 的独立端口配置，Linux继续使用当前三个 PTY。
6. 增加协议级测试：分帧、超长行、非法参数、忙状态、暂停/继续、停止和串口断开重连。

## 7. 验收标准

- 原 RK 左、右协议命令与应答保持兼容。
- 运动阶段两路 RK 均能收到相同的 20 Hz 实际转速帧；暂停或停止后不再持续发送运行帧。
- USB 能独立完成状态查询、任务管理和运行控制，每条命令都有确定响应。
- USB 控制不会绕过原安全状态机，也不会阻塞电机控制循环。
- Linux、Windows模拟器和 AT32 共用协议层；仅串口底层实现不同。
- 三路同时工作时不存在帧内容交叉、缓冲区越界或控制线程死锁。

## 8. 当前需要特别注意的代码现状

- 电机只有一个已确认的实际速度源，因此本文默认向两个 RK 广播同一速度。
- Windows当前仅初始化 RK 右侧 `COM1`，尚未真正实现三路独立桌面串口。
- Linux USB 命令处理目前只提供基础响应，AT32 的完整诊断命令仍由 MCU 条件代码实现；统一命令表需要后续重构。
- RK 接收以全局缓冲区和标志位实现，速度发送与接收可以并行，但多发送源必须串行化。
- 现有 USB日志和命令响应共用同一字节流，正式上位机协议必须使用 `ok/err/data/log` 前缀区分消息类型。
