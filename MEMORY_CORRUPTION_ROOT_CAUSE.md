# 真正的内存破坏问题分析

## 关键发现

### 1. 问题特征
- **少量操作（<20次）**: 正常
- **多次操作（≥20次）**: `Task_Btn_Array[i]->parent` 被破坏
- **触发位置**: LVGL 刷新过程中 (`refr_obj_and_children`)

### 2. 内存布局

```
地址范围                  | 内容                    | 大小
------------------------|------------------------|--------
0x20000000-0x20000090   | .data 段开始            |
0x20000090              | adc1_ordinary_valuetab | 4 bytes (DMA缓冲区!)
...
0x20050010-0x2005F010   | LVGL 堆                 | 60KB
...
0x2005bf38              | Task_Btn_Array         | 16 bytes
0x2005FFF0              | RAM 结束                |
```

### 3. 危险的 DMA 配置

**inc_motor.c 第 41 行:**
```c
__IO uint16_t adc1_ordinary_valuetab[2] = {0};
```

**DMA 配置 (第 47 行):**
```c
dma_init_struct.memory_base_addr = (uint32_t)adc1_ordinary_valuetab;  // 0x20000090
dma_init_struct.loop_mode_enable = TRUE;  // 循环模式！
```

**注意到问题了吗？**

虽然 `adc1_ordinary_valuetab` 在 0x20000090（远离 LVGL 堆），但是：

## 可能的破坏机制

### 假设 1: DMA 配置错误导致越界

如果 DMA 配置有任何错误（地址计算、传输长度、对齐），DMA 可能写入到错误的内存位置。

**多次操作后出现问题的原因:**
- DMA 累积错误
- 某个条件触发后 DMA 地址计算错误

### 假设 2: 栈溢出

```
Stack_Size EQU 0x00000400  ; 只有 1KB!
```

**这个栈太小了！** LVGL + FreeRTOS + 递归调用很容易溢出。

**多次操作后出现问题**是典型的**栈累积溢出**特征：
- 第 1-19 次: 栈使用量递增但未溢出
- 第 20 次: 栈溢出，覆盖了 LVGL 堆中的对象

### 假设 3: 堆碎片化导致的越界

多次 alloc/free 后:
- LVGL 堆碎片化严重
- 某次分配返回的地址超出预期
- 写入操作越界

## 验证方法

### 1. 检查栈使用量

在代码中添加栈水印检测：

```c
// 在 main() 开始处
extern uint32_t Image$$RW_IRAM1$$ZI$$Limit;
extern uint32_t __initial_sp;

void check_stack_usage(void) {
    uint32_t stack_top = (uint32_t)&__initial_sp;
    uint32_t current_sp;
    __asm volatile ("MOV %0, sp" : "=r" (current_sp));
    uint32_t used = stack_top - current_sp;
    
    if(used > 0x300) {  // 超过 75%
        volatile int trap = 1;
        while(trap);  // 栈即将溢出！
    }
}
```

在关键位置调用此函数（如 LVGL timer 回调中）。

### 2. 增加栈大小

```asm
Stack_Size EQU 0x00002000  ; 增加到 8KB
```

**如果增加栈后问题消失，就是栈溢出！**

### 3. 禁用 ADC DMA

临时禁用 `adc_config()` 的调用，看问题是否消失：

```c
void motor_init(void)
{
    gpio_config();
    // adc_config();  // <-- 注释掉
    tmr_both_init();
    Motor_Ctl_Set(0,0);
}
```

### 4. 启用 MPU (Memory Protection Unit)

配置 MPU 保护 LVGL 堆区域，只允许 LVGL 代码访问。

## 建议的修复步骤

### 立即尝试:

1. **增加栈大小到 8KB**（最可能的原因）
2. 在每次操作后调用 `check_stack_usage()`
3. 观察是否还出现问题

### 如果问题仍然存在:

4. 启用 `LV_USE_ASSERT_MEM_INTEGRITY`
5. 启用 `LV_USE_LOG` 并设置为 `LV_LOG_LEVEL_TRACE`
6. 检查是否有 DMA 相关的异常

## 最可能的根本原因

**栈溢出 (95% 确信)**

证据:
- ? 多次操作后才出现（栈累积）
- ? 1KB 栈对于 LVGL+FreeRTOS 太小
- ? `refr_obj_and_children` 是递归函数，栈消耗大
- ? 栈向下增长，会覆盖 BSS 段（Task_Btn_Array 在 .bss）

**立即修复: 增加栈大小到至少 4KB！**
