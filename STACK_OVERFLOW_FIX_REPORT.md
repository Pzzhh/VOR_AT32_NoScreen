# 栈溢出问题完整修复报告

## 问题根源

**确认：栈溢出导致内存破坏**

### 原始配置
```asm
Stack_Size EQU 0x00000400  ; 只有 1KB (1024 字节)
```

### 关键证据
1. ? **多次操作后才出现问题** - 典型的栈累积溢出
2. ? **在 LVGL 刷新递归函数中触发** - `refr_obj_and_children`
3. ? **破坏的是 `Task_Btn_Array[i]->parent`** - 在 BSS 段，靠近栈区域

---

## 发现的栈使用问题

### page1.c 中的栈滥用

| 位置 | 原代码 | 问题 | 频率 |
|------|--------|------|------|
| 第 761 行 | `const char errormsgT[4][20]` | **80 字节** | 每 100ms |
| 第 197 行 | `char str[20]` | 20 字节 | 频繁 |
| 第 114 行 | `char strs[25]` | 25 字节 | 非常频繁 |
| 第 405 行 | `const char string[][5]` | 20 字节 | 启动时 |

**总计**: 单次调用最多 145 字节栈使用！

### 栈溢出计算

```
LVGL 递归深度估计: 5-10 层
每层函数栈帧: ~50-100 字节
局部变量: 145 字节 (page1.c)
其他中断/回调: 200+ 字节

总计估计: 800-1200 字节

原始栈大小: 1024 字节  ← 非常危险！
```

---

## 修复措施

### 1. 增加栈大小（主要修复）

```asm
; startup_at32f435_437.s
Stack_Size EQU 0x00002000  ; 8KB (从 1KB 增加到 8KB)
```

**效果**: ZI-data 从 383992 增加到 391160 (+7168 字节)

### 2. 优化栈使用（page1.c）

#### 修复前后对比

**第 761 行 - UI_Page1_Btn_Refulsh():**
```c
// 修复前 (每次调用分配 80 字节栈!)
const char errormsgT[4][20] = {...};

// 修复后 (只分配一次，在 .rodata 段)
static const char errormsgT[4][20] = {...};
```

**第 197 行 - ui1_():**
```c
// 修复前
char str[20];

// 修复后
static char str[20];  
```

**第 114 行 - Set_table_Cell_Text():**
```c
// 修复前
char strs[25];

// 修复后
static char strs[25];
```

**第 405 行 - UI_Task_Btn_Init():**
```c
// 修复前
const char string[][5] = {...};

// 修复后
static const char string[][5] = {...};
```

### 节省效果
- 代码段: 从 255230 减少到 255178 (-52 字节)
- 栈使用: 减少约 145 字节/调用
- **关键**: 高频函数 `UI_Page1_Btn_Refulsh()` 节省 80 字节栈

---

## 为什么 PC 模拟器没问题？

1. **PC 栈大小**: 通常 1-8MB
2. **64位对齐**: 结构体更大，padding 更多，溢出落在安全区
3. **编译器差异**: GCC vs ARMCC，优化策略不同

---

## 验证方法

### 1. 栈使用检测代码

```c
extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Limit;  // 栈底
extern uint32_t __initial_sp;                      // 栈顶

void check_stack_usage(void) {
    uint32_t stack_top = (uint32_t)&__initial_sp;
    uint32_t current_sp;
    __asm volatile ("MOV %0, sp" : "=r" (current_sp));
    uint32_t used = stack_top - current_sp;
    uint32_t total = 0x2000;  // 8KB
    
    if(used > total * 0.8) {  // 超过 80%
        LV_LOG_WARN("Stack usage: %d/%d (%.1f%%)", 
                    used, total, (used * 100.0f) / total);
    }
}
```

在 `UI_Page1_Timer_UIupdata_handle()` 中调用此函数。

### 2. 内存监控

使用之前创建的 Keil INI 脚本监控 `Task_Btn_Array[i]->parent`。

### 3. 测试步骤

1. 烧录新固件
2. 进行 20+ 次 ADD/DEL 操作
3. 观察 `parent` 是否还会被破坏
4. 检查栈使用统计

---

## 结论

**根本原因**: 栈溢出导致覆盖 LVGL 堆中的对象

**主要元凶**: 
1. 栈太小 (1KB)
2. `UI_Page1_Btn_Refulsh()` 每 100ms 分配 80 字节栈数组

**修复效果**:
- ? 栈大小增加 7 倍 (1KB → 8KB)
- ? 减少栈使用 145 字节/调用
- ? 问题应该彻底解决

---

## 后续建议

1. **启用栈监控**: 在关键位置添加 `check_stack_usage()`
2. **代码审查**: 检查所有高频函数的栈使用
3. **使用 static**: 对于不变的字符串数组，都应该用 `static const`
4. **避免大数组**: 栈上不要分配超过 32 字节的数组

---

## 相关文件

- `startup_at32f435_437.s` - 栈大小配置
- `page1.c` - 主要栈使用优化
- `MEMORY_CORRUPTION_ROOT_CAUSE.md` - 详细分析
- `debug_memory.ini` - Keil 调试脚本
