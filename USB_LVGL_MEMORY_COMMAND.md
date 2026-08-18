# USB LVGL 内存分析命令使用说明

## ? 概述

通过 USB 虚拟串口（CDC ACM）输入命令来分析 LVGL 内存使用情况，无需修改内存大小即可进行诊断。

## ? 功能特性

- ? 实时内存使用情况查询
- ? 碎片率分析
- ? 内存块统计
- ? 警告提示（高碎片率、低内存）
- ? 无需重新编译即可查看状态

## ? 支持的命令

### 1. `lvmem` - LVGL 内存分析

显示详细的 LVGL 内存使用情况：

```
============ LVGL Memory Analysis ============
Total Size:      46080 bytes (45 KB)
Used:            38500 bytes (37 KB)
Free:            7580 bytes (7 KB)
Max Used:        39000 bytes (38 KB)
Used Percent:    83%
Fragmentation:   25%
Free Blocks:     12
Used Blocks:     45
Biggest Free:    2048 bytes
WARNING: High fragmentation!
==============================================
```

**字段说明：**
- **Total Size**: 内存池总大小
- **Used**: 当前已使用内存
- **Free**: 当前空闲内存
- **Max Used**: 历史最大使用量
- **Used Percent**: 使用率百分比
- **Fragmentation**: 碎片率（越低越好）
- **Free Blocks**: 空闲内存块数量（过多表示碎片化严重）
- **Used Blocks**: 已使用内存块数量
- **Biggest Free**: 最大连续可用内存块

**警告条件：**
- 碎片率 > 50% → 显示 "High fragmentation!"
- 可用内存 < 10% → 显示 "Low free memory!"
- 峰值使用 > 90% → 显示 "Peak usage >90%!"

### 2. `help` - 帮助信息

显示所有可用命令列表

## ?? 使用方法

### 方式 1: 使用串口终端工具

1. 打开串口终端（如 PuTTY, SecureCRT, Tera Term）
2. 配置参数：
   - 波特率：115200
   - 数据位：8
   - 停止位：1
   - 校验位：无
   - 流控：无

3. 连接到 USB CDC 设备
4. 输入命令 + 回车：
   ```
   lvmem
   ```

### 方式 2: 使用 Python 脚本

```python
import serial

# 打开串口
ser = serial.Serial('COM3', 115200, timeout=1)

# 发送命令
ser.write(b'lvmem\r\n')

# 读取响应
while True:
    line = ser.readline().decode('utf-8', errors='ignore')
    if line:
        print(line, end='')
    else:
        break

ser.close()
```

### 方式 3: Linux/Mac 终端

```bash
# 查看设备
ls /dev/tty*

# 发送命令
echo "lvmem" > /dev/ttyACM0

# 读取响应
cat /dev/ttyACM0
```

## ? 分析示例

### 示例 1: 正常状态
```
Total Size:      46080 bytes (45 KB)
Used:            12000 bytes (11 KB)
Free:            34080 bytes (33 KB)
Fragmentation:   5%
```
? **健康** - 使用率 26%，低碎片率

### 示例 2: 内存紧张
```
Total Size:      46080 bytes (45 KB)
Used:            42000 bytes (41 KB)
Free:            4080 bytes (3 KB)
Fragmentation:   15%
WARNING: Low free memory (<10%)!
```
?? **需要注意** - 可用内存不足 10%，建议增加内存池大小

### 示例 3: 高碎片化
```
Total Size:      46080 bytes (45 KB)
Used:            30000 bytes (29 KB)
Free:            16080 bytes (15 KB)
Fragmentation:   65%
Free Blocks:     48
WARNING: High fragmentation!
```
? **严重问题** - 碎片率超过 50%，虽然有可用内存但无法分配大块连续内存

## ? 故障诊断

### 问题 1: 输入命令无响应

**可能原因：**
- USB 未正确连接
- 驱动未安装
- 串口被其他程序占用

**解决方法：**
```c
// 检查 USB 初始化状态
if (!HAL_USB_ACM_IsReady()) {
    // USB 未就绪
}
```

### 问题 2: 碎片率持续升高

**原因：** 频繁的小块分配/释放

**解决方法：**
1. 增加内存池大小（从 45KB → 128KB）
2. 优化对象创建/销毁逻辑
3. 使用对象池预分配

### 问题 3: 内存泄漏

**现象：** Used 持续增长，Free 持续减少

**检查方法：**
```bash
# 多次执行 lvmem 命令，对比 Max Used
lvmem
# 等待 10 秒
lvmem
# 如果 Max Used 持续增长 → 可能存在泄漏
```

**排查步骤：**
1. 检查是否有未释放的 lv_obj
2. 检查是否有未释放的 style
3. 使用 `lv_obj_del()` 正确删除对象

## ?? 代码实现位置

| 文件 | 函数 | 说明 |
|------|------|------|
| `communication_API.c` | `HAL_USB_LVGL_Memory_Analysis()` | 内存分析函数 |
| `communication_API.c` | `HAL_USB_Command_Process()` | 命令解析器 |
| `main.c` | `LVGL_HANDLER()` | 命令接收处理 |

## ? 扩展命令（未来）

可以添加更多命令：

```c
// 示例：添加清除统计命令
else if (strcmp(cmd_buf, "lvmemclear") == 0)
{
    // 重置 max_used 等统计信息
    HAL_USB_Log_Printf("Memory statistics cleared.\r\n");
}

// 示例：添加内存转储命令
else if (strcmp(cmd_buf, "lvmemdump") == 0)
{
    // 输出内存块详细信息
    HAL_USB_Log_Printf("Memory block dump:\r\n");
    // ... 实现内存块遍历
}
```

## ?? 注意事项

1. **线程安全**: 当前实现在 LVGL 任务中调用，确保不会并发访问
2. **性能影响**: `lv_mem_monitor()` 会遍历内存块，频繁调用可能影响性能
3. **缓冲区大小**: 命令行缓冲区限制 64 字节
4. **大小写敏感**: 命令区分大小写（全小写）

## ? 推荐使用场景

- ? 开发阶段调试内存问题
- ? 现场问题诊断（无需重新烧录）
- ? 长期运行稳定性测试
- ? 性能优化前后对比

## ? 相关文档

- [LVGL Memory Management](https://docs.lvgl.io/master/overview/memory.html)
- [LVGL TLSF Allocator](https://github.com/lvgl/lvgl/blob/master/src/misc/lv_tlsf.c)
- USB_ACM_IMPLEMENTATION_SUMMARY.md
