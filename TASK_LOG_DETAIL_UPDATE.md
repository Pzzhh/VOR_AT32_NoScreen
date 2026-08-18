# 任务日志详情功能更新说明

## ? 更新内容

现在USB ACM日志输出包含**完整的任务详细信息**，类似于 `PrintfInfo()` 函数的格式，但不包含时间戳。

## ? 新功能对比

### 之前的日志格式
```
[TASK] ADD index=0, mode=1
[TASK] SAVE index=1, mode=2
```

### 现在的日志格式（详细版）
```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
[TASK] ADD index=1, CONT: S60 V45
[TASK] ADD index=2, OVAR: S30 V40 I15
[TASK] ADD index=3, VHIT: T20 CW
[TASK] ADD index=4, TC: S50 V35
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8
```

## ? 支持的任务类型详情

| 任务类型 | 日志格式 | 示例 |
|---------|---------|------|
| **VOR** | `VOR: <模式> T<次数> V<速度> F<频率>` | `VOR: VOR T10 V30 F0.5` |
| | 模式: VOR / OKR / VOR_OKR / VOR_OKRR | `VOR: VOR_OKR T15 V35 F0.8` |
| **CONT** | `CONT: S<秒数> V<速度>` | `CONT: S60 V45` |
| **OVAR** | `OVAR: S<秒数> V<速度> I<角度>` | `OVAR: S30 V40 I15` |
| **VHIT** | `VHIT: T<次数> <方向>` | `VHIT: T20 CW` |
| | 方向: CW (顺时针) / CCW (逆时针) | `VHIT: T15 CCW` |
| **TC** | `TC: S<次数> V<速度>` | `TC: S50 V35` |

## ? 技术实现

### 新增函数

```c
// 生成任务详情字符串（不含时间戳）
static void Task_Info_ToString(void* task_ptr, char* buffer, uint16_t bufsize);

// 记录带完整任务详情的日志
void HAL_USB_Log_Task_Detail(const char* operation, uint16_t index, void* task);
```

### 使用方式

```c
// 在 task_manager.c 中使用
Task_Parameter_Struct task_info;

// ADD操作 - 输出完整任务详情
HAL_USB_Log_Task_Detail("ADD", index, &task_info);
// 输出: [TASK] ADD index=0, VOR: VOR T10 V30 F0.5

// SAVE操作 - 输出完整任务详情
HAL_USB_Log_Task_Detail("SAVE", index, &task_info);
// 输出: [TASK] SAVE index=1, CONT: S60 V45
```

## ? 完整示例输出

```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
[TASK] ADD index=1, CONT: S60 V45
[TASK] ADD index=2, OVAR: S30 V40 I15
[TASK] ADD index=3, VHIT: T20 CW
[TASK] ADD index=4, TC: S50 V35
[TASK] MOVE index=1, from=1 to=2
[TASK] SAVE index=0, VOR: VOR_OKR T15 V35 F0.8
[TASK] SAVE_CREATE index=5, CONT: S120 V50
[TASK] DEL index=2, Success
[TASK] RELOAD: Clearing all tasks
[TASK] RELOAD: Successfully loaded 5 tasks
```

## ? 参数说明

### VOR 任务
- **模式** (ExMode): VOR(0), OKR(1), VOR_OKR(2), VOR_OKRR(3)
- **T**: 次数 (Counter)
- **V**: 速度 (Vel)
- **F**: 频率 (Freq)

### CONT 任务
- **S**: 秒数 (Sec)
- **V**: 速度 (Vel)

### OVAR 任务
- **S**: 秒数 (Sec)
- **V**: 速度 (Vel)
- **I**: 倾斜角度 (Inc_Degree)

### VHIT 任务
- **T**: 次数 (Counter)
- **方向**: CW=顺时针, CCW=逆时针 (IsCW)

### TC 任务
- **S**: 次数 (Counter)
- **V**: 速度 (Vel)

## ? 与 PrintfInfo 的对比

### PrintfInfo() 函数格式
```
20210101093100_mouse_VOR_T10V30F05
```
- 包含时间戳
- 包含鼠标名称
- 用于RK3588文件命名

### USB日志格式
```
[TASK] ADD index=0, VOR: VOR T10 V30 F0.5
```
- 无时间戳
- 无鼠标名称
- 包含操作类型和索引
- 用于USB串口调试监控

## ? 优势

1. **实时监控**: 在串口终端实时查看任务参数
2. **调试方便**: 清晰显示每个任务的具体配置
3. **易于理解**: 人类可读的格式，不需要查表
4. **完整信息**: 包含所有任务关键参数
5. **无需时间戳**: 专注于任务信息本身

## ? 相关文档

- `USB_ACM_LOG_README.md` - 完整使用说明
- `USB_ACM_IMPLEMENTATION_SUMMARY.md` - 实现总结
- `task_control.c` - PrintfInfo() 参考实现

## ? 更新状态

- ? Task_Info_ToString() 函数实现
- ? HAL_USB_Log_Task_Detail() 函数实现
- ? ADD 操作使用详细日志
- ? SAVE 操作使用详细日志
- ? SAVE_CREATE 操作使用详细日志
- ? 文档更新完成

**所有任务操作现在都输出完整的详细信息！** ?
