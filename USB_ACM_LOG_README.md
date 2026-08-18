# USB ACM 虚拟串口日志功能说明

## 功能概述

本项目已完善USB ACM (Abstract Control Model) 虚拟串口功能，并为任务管理操作添加了日志输出功能。

## 新增API函数

### 1. USB ACM 基础函数

```c
// 初始化USB ACM虚拟串口
uint8_t HAL_USB_ACM_Init(void);

// 发送数据到USB ACM
// 返回: 1=成功, 0=失败或忙碌
uint8_t HAL_USB_ACM_Send(uint8_t* data, uint16_t len);

// 从USB ACM读取数据
// 返回: 读取到的字节数
uint16_t HAL_USB_ACM_Read(uint8_t* buffer, uint16_t maxlen);

// 检查USB ACM是否就绪
// 返回: 1=就绪, 0=未就绪
uint8_t HAL_USB_ACM_IsReady(void);
```

### 2. USB ACM 日志函数

```c
// 格式化输出日志到USB ACM
void HAL_USB_Log_Printf(const char* format, ...);

// 记录任务操作日志（简单版本）
void HAL_USB_Log_Task_Operation(const char* operation, uint16_t index, const char* details);

// 记录任务操作日志（带完整任务详情）
void HAL_USB_Log_Task_Detail(const char* operation, uint16_t index, void* task);
```

## 任务操作日志格式

所有任务操作（ADD, DEL, MOVE, SAVE）都会自动输出详细日志到USB ACM虚拟串口。

### 日志格式示例

**ADD操作 - 带完整任务详情：**
```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
[TASK] ADD index=1, CONT: S60 V45
[TASK] ADD index=2, OVAR: S30 V40 I15
[TASK] ADD index=3, VHIT: T20 CW
[TASK] ADD index=4, TC: S50 V35
```

**SAVE操作 - 带完整任务详情：**
```
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8
[TASK] SAVE index=1, CONT: S120 V50
```

**其他操作：**
```
[TASK] DEL index=1, Success
[TASK] MOVE index=0, from=0 to=1
[TASK] RELOAD: Clearing all tasks
[TASK] RELOAD: Successfully loaded 5 tasks
[TASK] ADD_FAILED index=8, Task list full
[TASK] SAVE_CREATE index=5, OVAR: S45 V38 I20
```

### 任务详情格式说明

- **VOR**: `VOR/OKR/VOR_OKR/VOR_OKRR T<次数> V<速度> F<频率>`
- **CONT**: `CONT: S<秒数> V<速度>`
- **OVAR**: `OVAR: S<秒数> V<速度> I<倾斜角度>`
- **VHIT**: `VHIT: T<次数> CW/CCW`
- **TC**: `TC: S<次数> V<速度>`

## 使用方法

### 1. 初始化

USB ACM会在 `HAL_communication_Init()` 中自动初始化：

```c
void HAL_communication_Init(void)
{
    Rk3588_Init();
    HAL_communication_Mpu_Init();
    HAL_communication_Bl_Init();
    HAL_BL_USART_INIT();
    HAL_USB_ACM_Init(); // 自动初始化USB ACM
}
```

### 2. 查看日志

使用串口终端软件（如PuTTY, Tera Term, CoolTerm等）连接到USB虚拟串口：

- **波特率**: 115200 (默认，可通过USB设置改变)
- **数据位**: 8
- **停止位**: 1
- **校验位**: None
- **流控制**: None

### 3. 自定义日志输出

在代码中任意位置使用日志函数：

```c
// 基础日志输出
HAL_USB_Log_Printf("System initialized at %d ms\r\n", ControlGetTick());

// 任务操作日志
HAL_USB_Log_Task_Operation("CUSTOM_OP", task_index, "Custom operation details");
```

## 已实现的日志记录点

### 1. ADD 操作
- 成功添加: `[TASK] ADD index=X, <任务详情>`
  - 示例: `[TASK] ADD index=0, VOR: VOR T10 V30 F0.5`
- 列表已满: `[TASK] ADD_FAILED index=X, Task list full`
- 插入失败: `[TASK] ADD_FAILED index=X, Insert failed`

### 2. DEL 操作
- 成功删除: `[TASK] DEL index=X, Success`
- 删除失败: `[TASK] DEL_FAILED index=X, Delete failed`

### 3. MOVE 操作
- 成功移动: `[TASK] MOVE index=X, from=X to=Y`

### 4. SAVE 操作
- 成功保存: `[TASK] SAVE index=X, <任务详情>`
  - 示例: `[TASK] SAVE index=0, CONT: S60 V45`
- 索引超范围: `[TASK] SAVE_FAILED index=X, Index out of range`
- 创建新任务: `[TASK] SAVE_CREATE index=X, <任务详情>`
- 创建失败: `[TASK] SAVE_CREATE_FAILED index=X, Insert failed`

### 5. RELOAD 操作
- 开始清空: `[TASK] RELOAD: Clearing all tasks`
- 加载成功: `[TASK] RELOAD: Successfully loaded X tasks`
- 加载失败: `[TASK] RELOAD_FAILED: Only loaded X/Y tasks`

## 技术细节

### USB CDC 设备信息

- **厂商**: Artery
- **产品名**: AT32 Virtual Com Port
- **配置**: Virtual ComPort Config
- **接口**: Virtual ComPort Interface

### 实现位置

1. **USB底层驱动**: `commuication/USB/usbd_class/cdc/cdc_class.c`
2. **USB初始化**: `commuication/USB/usb_init.c`
3. **API接口**: `commuication/communication_API.c/h`
4. **任务管理日志**: `INTERFACE/UI/Task/task_manager.c`

### 缓冲区大小

- 发送缓冲区: 256 字节 (日志格式化)
- USB端点大小: 定义在 `cdc_class.h`
  - `USBD_CDC_OUT_MAXPACKET_SIZE`: 接收包大小
  - `USBD_CDC_IN_MAXPACKET_SIZE`: 发送包大小

## 注意事项

1. **日志输出不会阻塞**: 如果USB忙碌，日志会被丢弃
2. **仅在AT32F43平台有效**: 使用 `#ifdef ATM32F43_xx` 条件编译
3. **自动初始化**: Communication线程会自动初始化USB ACM
4. **线程安全**: 日志输出函数可在任意线程中调用

## 调试技巧

### 1. 检查USB连接状态

```c
if (HAL_USB_ACM_IsReady())
{
    HAL_USB_Log_Printf("USB is ready\r\n");
}
```

### 2. 发送自定义数据

```c
uint8_t data[] = "Hello USB!\r\n";
HAL_USB_ACM_Send(data, sizeof(data)-1);
```

### 3. 读取数据

```c
uint8_t buffer[64];
uint16_t len = HAL_USB_ACM_Read(buffer, sizeof(buffer));
if (len > 0)
{
    // 处理接收到的数据
}
```

## 示例输出

```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
[TASK] ADD index=1, CONT: S60 V45
[TASK] ADD index=2, OVAR: S30 V40 I15
[TASK] MOVE index=1, from=1 to=2
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8
[TASK] DEL index=2, Success
[TASK] RELOAD: Clearing all tasks
[TASK] RELOAD: Successfully loaded 3 tasks
System message: Task operation completed
```

## 扩展建议

1. 可以添加时间戳到日志输出
2. 可以添加更详细的任务参数信息
3. 可以实现日志等级过滤（DEBUG, INFO, WARNING, ERROR）
4. 可以添加二进制数据输出支持

## 相关文件

- `commuication/communication_API.c` - USB ACM实现
- `commuication/communication_API.h` - API头文件
- `commuication/USB/usb_init.c` - USB初始化
- `INTERFACE/UI/Task/task_manager.c` - 任务管理器（带日志）
