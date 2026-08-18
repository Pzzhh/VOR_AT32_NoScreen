# 内存断点误报分析

## 问题现象

设置内存写入断点监控 `Task_Btn_Array[i]->parent` 时，在以下代码处触发：

```c
// lv_refr.c 第 789 行
lv_obj_t * child = parent->spec_attr->children[i];
```

## 原因分析

### 这不是真正的 parent 被修改！

**误报的根本原因：**

1. 监控的地址是：`Task_Btn_Array[i] + 4` (即 obj->parent 成员的地址)
2. 局部变量 `child` 在栈上分配
3. **栈地址可能与堆中的对象地址发生碰撞！**

### 为什么会发生地址碰撞？

AT32F435 的内存布局：
```
0x20000000 - 0x20050000  主RAM (320KB)
0x20050010 - 0x2005F010  LVGL堆 (60KB) <-- Task_Btn_Array 对象在这里
```

栈从高地址向下增长，如果栈使用较多，可能进入 LVGL 堆的地址范围。

### 真实情况

```c
lv_obj_t * child = parent->spec_attr->children[i];
```

这行代码的操作：
1. **读取** `parent->spec_attr->children[i]` 的值（一个指针）
2. **写入** 到栈上的局部变量 `child`

如果 `&child` 的地址恰好等于某个被监控的 `Task_Btn_Array[i]->parent` 地址，就会触发断点！

## 验证方法

当断点触发时，在 Keil Command 窗口执行：

```
print &child                    // 查看局部变量 child 的地址
print Task_Btn_Array[0]->parent // 查看对象 parent 的当前值
print &(Task_Btn_Array[0]->parent) // 查看 parent 成员的地址
```

**如果 `&child` == `&(Task_Btn_Array[i]->parent)`，就是误报！**

## 解决方案

### 方案 1: 使用条件断点

在 Keil 中设置断点条件，只在 PC 指向真正修改 parent 的指令时才中断。

### 方案 2: 代码中加入检测

在 `Task_Btn_Array` 使用的关键位置添加检测代码：

```c
void check_task_btn_parent(void) {
    for(int i = 0; i < 4; i++) {
        if(Task_Btn_Array[i] != NULL) {
            lv_obj_t *expected_parent = ...; // 预期的 parent
            if(Task_Btn_Array[i]->parent != expected_parent) {
                // Parent 被意外修改！
                volatile int trap = 1;
                while(trap);  // 在此处设置断点
            }
        }
    }
}
```

在关键位置调用此函数：
- LVGL timer 回调中
- 事件处理函数返回前
- 样式修改操作后

### 方案 3: 增加栈空间（减少碰撞概率）

修改 `startup_at32f435_437.s`:
```asm
Stack_Size EQU 0x00001000  ; 从 0x400 增加到 4KB
```

### 方案 4: 使用软件断点而非硬件断点

在可能修改 parent 的函数入口设置普通断点，检查前后值。

## 推荐做法

**如果你 100% 确定 parent 不会被修改**，那么这次断点触发很可能是误报。

建议：
1. 在断点触发时，检查 `&child` 和被监控地址是否相同
2. 如果相同，说明是栈变量地址碰撞，继续运行
3. 如果不同，才是真正的 parent 被修改

## 如何找到真正的问题

如果断点总是在 `refr_obj_and_children` 触发且是误报，可以：

1. 在代码关键位置手动检查 parent 值
2. 在 LVGL 对象创建/删除时记录日志
3. 使用 Memory Compare 功能周期性检查内存
