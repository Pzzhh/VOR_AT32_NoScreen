# USB ACM 虚拟串口功能完善总结

## ? 已完成的工作

### 1. 完善 USB ACM 读写函数 ?

**新增API函数** (`communication_API.c/h`):

```c
// USB ACM 基础功能
uint8_t HAL_USB_ACM_Init(void);           // 初始化
uint8_t HAL_USB_ACM_Send(uint8_t* data, uint16_t len);  // 发送
uint16_t HAL_USB_ACM_Read(uint8_t* buffer, uint16_t maxlen); // 读取
uint8_t HAL_USB_ACM_IsReady(void);        // 状态检查

// USB ACM 日志功能
void HAL_USB_Log_Printf(const char* format, ...);  // 格式化日志
void HAL_USB_Log_Task_Operation(const char* operation, uint16_t index, const char* details); // 简单日志
void HAL_USB_Log_Task_Detail(const char* operation, uint16_t index, void* task); // 详细日志
```

**实现细节**:
- 基于现有的 `USB_CDC_Send()` 和 `USB_CDC_GET()` 封装
- 支持 printf 风格的格式化输出
- 256字节日志缓冲区
- 非阻塞设计（USB忙碌时自动丢弃）
- **新增任务详情解析**：自动解析任务参数并格式化输出

### 2. 为任务操作添加 USB 日志输出 ?

**已添加日志的操作**:

#### ADD 操作
```c
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5              // VOR任务
[TASK] ADD index=1, CONT: S60 V45                       // 连续任务
[TASK] ADD index=2, OVAR: S30 V40 I15                   // OVAR任务
[TASK] ADD index=3, VHIT: T20 CW                        // VHIT任务
[TASK] ADD index=4, TC: S50 V35                         // TC任务
[TASK] ADD_FAILED index=8, Task list full               // 失败
```

#### DEL 操作
```c
[TASK] DEL index=1, Success             // 成功
[TASK] DEL_FAILED index=10, Delete failed  // 失败
```

#### MOVE 操作
```c
[TASK] MOVE index=0, from=0 to=1        // 移动信息
```

#### SAVE 操作
```c
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8         // 保存已有任务
[TASK] SAVE_CREATE index=5, CONT: S120 V50             // 创建新任务
[TASK] SAVE_FAILED index=12, Index out of range        // 失败
```

#### RELOAD 操作
```c
[TASK] RELOAD: Clearing all tasks
[TASK] RELOAD: Successfully loaded 5 tasks
[TASK] RELOAD_FAILED: Only loaded 3/5 tasks
```

### 3. 修改的文件清单

| 文件 | 修改内容 |
|------|---------|
| `commuication/communication_API.h` | 新增USB ACM函数声明 |
| `commuication/communication_API.c` | 实现USB ACM读写和日志函数 |
| `INTERFACE/UI/Task/task_manager.c` | 为所有任务操作添加日志输出 |
| `USB_ACM_LOG_README.md` | 创建使用说明文档 |

## ? 功能说明

### USB 虚拟串口连接方式

1. **连接设备**: 通过USB线连接AT32开发板到PC
2. **识别端口**: Windows会识别为虚拟串口（COM口）
3. **打开终端**: 使用串口工具（PuTTY/Tera Term等）
4. **配置参数**:
   - 波特率: 115200
   - 数据位: 8
   - 停止位: 1
   - 校验: None

### 自动初始化流程

```
HAL_communication_Init()
    ↓
HAL_USB_ACM_Init()
    ↓
USB1_Init()
    ↓
usbd_init() with CDC class
```

### 日志输出示例

实际运行时的日志输出：

```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
[TASK] ADD index=1, CONT: S60 V45
[TASK] ADD index=2, OVAR: S30 V40 I15
[TASK] MOVE index=1, from=1 to=2
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8
[TASK] DEL index=2, Success
[TASK] RELOAD: Clearing all tasks
[TASK] RELOAD: Successfully loaded 3 tasks
```

### 任务详情格式

| 任务类型 | 格式 | 示例 |
|---------|------|------|
| VOR | `VOR/OKR/VOR_OKR/VOR_OKRR T<次数> V<速度> F<频率>` | `VOR: VOR T10 V30 F0.5` |
| CONT | `CONT: S<秒数> V<速度>` | `CONT: S60 V45` |
| OVAR | `OVAR: S<秒数> V<速度> I<倾斜角度>` | `OVAR: S30 V40 I15` |
| VHIT | `VHIT: T<次数> CW/CCW` | `VHIT: T20 CW` |
| TC | `TC: S<次数> V<速度>` | `TC: S50 V35` |

## ? 代码架构

```
USB底层 (usb_drivers)
    ↓
CDC类实现 (cdc_class.c)
    ↓  usb_vcp_send_data()
    ↓  usb_vcp_get_rxdata()
    ↓
USB初始化层 (usb_init.c)
    ↓  USB_CDC_Send()
    ↓  USB_CDC_GET()
    ↓
通信API层 (communication_API.c)
    ↓  HAL_USB_ACM_Send()
    ↓  HAL_USB_ACM_Read()
    ↓  HAL_USB_Log_Printf()
    ↓
应用层 (task_manager.c)
    ↓  HAL_USB_Log_Task_Operation()
```

## ? 关键技术点

### 1. 非阻塞发送
```c
// 检查USB是否就绪，不就绪则返回失败
if (!USB_init_flag)
    return 0;
```

### 2. 格式化输出
```c
// 使用vsnprintf实现printf风格输出
va_list args;
va_start(args, format);
int len = vsnprintf(log_buffer, sizeof(log_buffer), format, args);
va_end(args);
```

### 3. 任务信息记录
```c
// 自动解析并记录完整任务参数
// 无需手动构建详情字符串
HAL_USB_Log_Task_Detail("ADD", index, &task_struct);

// 输出示例：
// [TASK] ADD index=0, VOR: VOR T10 V30 F0.5
```

## ? 内存占用

- **日志缓冲区**: 256 bytes (栈上分配)
- **USB缓冲区**: 已在CDC类中定义
  - 接收缓冲区: `USBD_CDC_OUT_MAXPACKET_SIZE`
  - 发送缓冲区: 由USB驱动管理
- **总计**: < 512 bytes

## ?? 注意事项

1. **平台限制**: 仅在 `ATM32F43_xx` 平台有效
2. **非阻塞**: 日志发送失败不会重试
3. **线程安全**: 可在多线程环境中调用
4. **初始化**: 自动在communication线程中初始化

## ? 调试与测试

### 测试USB连接
```c
if (HAL_USB_ACM_IsReady()) {
    HAL_USB_Log_Printf("USB Ready!\r\n");
}
```

### 发送自定义日志
```c
HAL_USB_Log_Printf("System tick: %lu ms\r\n", ControlGetTick());
HAL_USB_Log_Printf("Task count: %d\r\n", Task_Stroage_GetSize());
```

### 读取命令（可扩展）
```c
uint8_t cmd[64];
uint16_t len = HAL_USB_ACM_Read(cmd, sizeof(cmd));
if (len > 0) {
    // 处理接收到的命令
}
```

## ? 扩展建议

### 短期扩展
- [ ] 添加时间戳到日志
- [ ] 实现日志等级（DEBUG/INFO/WARN/ERROR）
- [ ] 添加更多任务参数详情

### 长期扩展
- [ ] 双向命令通信
- [ ] 日志缓存与重传
- [ ] 二进制数据传输
- [ ] 远程配置功能

## ? 相关文档

- `USB_ACM_LOG_README.md` - 详细使用说明
- `commuication/USB/README.md` - USB驱动说明（如有）
- AT32 USB开发文档

## ? 使用示例

### 查看任务操作日志
```bash
# 在串口终端中观察（带详细任务信息）
1. 点击 "add" 按钮 → 显示 "[TASK] ADD index=X, VOR: VOR T10 V30 F0.5"
2. 点击 "del" 按钮 → 显示 "[TASK] DEL index=X, Success"
3. 点击 "move" 按钮 → 显示 "[TASK] MOVE index=X, from=X to=Y"
4. 点击 "save" 按钮 → 显示 "[TASK] SAVE index=X, CONT: S60 V45"
```

### 自定义日志应用
```c
// 在任意代码位置
HAL_USB_Log_Printf("Motor speed: %.2f RPM\r\n", motor_speed);
HAL_USB_Log_Printf("Sensor value: %d\r\n", sensor_reading);
```

## ? 完成状态

- ? USB ACM 读写函数完善
- ? ADD 操作日志
- ? DEL 操作日志  
- ? MOVE 操作日志
- ? SAVE 操作日志
- ? RELOAD 操作日志
- ? 使用说明文档
- ? 代码注释完整

**所有需求已完成！** ?
