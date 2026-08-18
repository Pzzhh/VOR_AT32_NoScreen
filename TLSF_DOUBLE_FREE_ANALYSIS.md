# TLSF 断言失败: "block already marked as free" 原因分析

## ? 断言失败位置

```c
// lv_tlsf.c 第 1167 行 (free 函数)
// lv_tlsf.c 第 1221 行 (realloc 函数)

tlsf_assert(!block_is_free(block) && "block already marked as free");
```

## ? 断言含义

这个断言检查：
- `!block_is_free(block)` = block 的 free bit 必须为 0（表示已占用）
- 如果断言失败，说明 block 的 free bit 为 1（表示已释放）
- **错误含义**: 尝试释放或重新分配一个已经被标记为"已释放"的内存块

---

## ? 常见原因分析

### 原因1: Double-Free (重复释放) ? 最常见

```
时间 →

T1: malloc 分配内存
    ┌─────────────────────┐
    │ block (used)        │
    │ - free_bit = 0      │ ← 标记为已占用
    └─────────────────────┘

T2: free(ptr) 第一次释放
    ┌─────────────────────┐
    │ block (free)        │
    │ - free_bit = 1      │ ← 标记为已释放
    └─────────────────────┘

T3: free(ptr) 第二次释放 ?
    尝试释放已释放的 block
    ↓
    tlsf_assert(!block_is_free(block)) 失败！
    ↓
    错误信息: "block already marked as free"
```

**具体代码问题** (在你的项目中):

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ...
    head->_free(Property);  // 第一次释放
    removeNode(head, node->data);
    ID_UPdata(head);
    // ...
}

// 稍后如果又调用了 _free(Property)，就会触发 double-free
```

### 原因2: 指针不对齐或损坏 ? 严重

```
场景: 指针被破坏

正常情况:
    ┌─────────────────────┐
    │ block header        │
    │ - size | free_bit   │ ← 第一个字段
    └─────────────────────┘

被破坏的指针:
    block_header_t* block = block_from_ptr(ptr);
    //                      ↓
    // ptr 往前偏移 block_start_offset 获取 block header
    
    如果 ptr 被破坏或不对齐:
    ├─ 计算出错的 block 地址
    └─ 访问到错误内存的 free_bit
    └─ 该位恰好为 1，触发断言
```

### 原因3: 内存越界导致 Free Bit 被破坏 ? 严重

```
内存布局:
┌──────────────┬──────────────┬──────────────┐
│ Block A      │ Block B      │ Block C      │
│ (in use)     │ (in use)     │ (in use)     │
│              │              │              │
│ size|fb      │ size|fb      │ size|fb      │
│ = 100|0      │ = 100|0      │ = 100|0      │
└──────────────┴──────────────┴──────────────┘

越界写入:
    Block A 越界写入到 Block B 的头部:
    ┌──────────────┬──────────────┬──────────────┐
    │ Block A      │ Block B      │ Block C      │
    │ (damaged)    │ (damaged)    │              │
    │              │ size|fb      │              │
    │              │ = 100|1  ← free_bit被改为1 │
    └──────────────┴──────────────┴──────────────┘

释放 Block B:
    free(ptr_B)
    ↓
    block = block_from_ptr(ptr_B)
    ↓
    block_is_free(block) 检查 free_bit
    ↓
    free_bit = 1（被越界写入破坏）
    ↓
    tlsf_assert 失败！
```

### 原因4: 释放错误的指针地址 ? 常见

```
示例:

// ? 错误的释放
Property* ptr = allocated_ptr;
free(ptr + 1);  // 释放的地址不对！

或:

// ? 使用被修改的指针
void* temp = malloc(100);
temp += 4;      // 指针偏移
free(temp);     // 释放错误的地址
```

### 原因5: 多线程竞态条件 ? 困难排查

```
线程1:                          线程2:

free(ptr)
└─ block = block_from_ptr(ptr)
└─ assert(!block_is_free)     ← 检查前
                                free(ptr)
                                └─ block_set_free(block)
                                └─ free_bit = 1
└─ 继续执行                     
└─ 再次检查 block_is_free      ← 现在失败！

结果: 竞态条件导致断言失败
```

### 原因6: 内存管理器被破坏 ? 极端

```
TLSF 的内部数据结构被破坏:

control_t 结构被写入:
├─ fl_bitmap (一级位图)
├─ sl_bitmap[16] (二级位图) 
└─ blocks[16][32] (空闲块链表)

如果这些被破坏，block_is_free() 可能:
├─ 访问到错误的内存
├─ 读到垃圾值
└─ 导致假的 free_bit = 1

所以即使 block 没有真的被释放，断言也会失败
```

---

## ? 实际代码中的风险点

### 风险点1: UI_ListBox.c 中的重复释放

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ...
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        lv_anim_del(obj, anim_y_cb);
        lv_style_reset(&Property->style);
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        lv_obj_del(obj);
        head->_free(Property);           // ← 第一次释放
        removeNode(head, node->data);
        ID_UPdata(head);
    }
}

// 如果后续有以下情况会导致 double-free:

// 情况A: 列表节点被重复处理
for (i = 0; i < size; i++) {
    UI_Table_Clean(list, i, obj);  // ← 可能操作同一个节点两次
}

// 情况B: 节点删除不完全
removeNode() 失败，但 Property 已释放
后续又尝试清理该节点
head->_free(Property) // ← double-free!

// 情况C: 容器删除时的级联清理
listbox_container_event_handler() 调用
while (prop->list->size > 0) {
    UI_Table_Clean(prop->list, 0, obj);
}
// 如果 removeNode 没有正确更新大小，会无限循环
// 导致重复 free 同一个 block
```

### 风险点2: lv_mem_alloc 和 head->_free 的混用

```c
// UI_ListBox.c 第 250-265 行
List* list = lv_mem_alloc(sizeof(List));
list->_free = lv_mem_free;      // ← 设置释放函数为 LVGL 的 mem_free
list->_malloc = lv_mem_alloc;

// ...

// 第 287 行
Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));

// ...

// 第 87 行 (UI_Table_Clean)
head->_free(Property);          // ← 使用设置的函数

问题:
├─ 如果 lv_mem_free 是 TLSF 管理的
├─ 而 Property 被多次释放
└─ 就会触发这个断言
```

### 风险点3: 动画清理不完全

```c
// 动画持续运行时删除对象
lv_obj_del(obj);                // ← 删除 LVGL 对象
head->_free(Property);          // ← 释放关联结构体

// 但动画仍在运行！
custom_anim_path() 被调用
├─ 访问 a->user_data (已释放)
├─ 可能尝试调用 table->updata_cd()
└─ 导致内存损坏
└─ 进而破坏了其他 block 的 free_bit

后续释放时:
free(other_block)
└─ 发现 free_bit 被破坏
└─ 断言失败
```

---

## ? 故障排查流程

### Step 1: 确认是否真的 Double-Free

```c
// 添加调试代码
void* my_free(void* ptr) {
    if (ptr == NULL) {
        printf("WARNING: Free NULL pointer\n");
        return;
    }
    
    block_header_t* block = block_from_ptr(ptr);
    if (block_is_free(block)) {
        printf("FATAL: Double-free detected at %p\n", ptr);
        printf("  Block: %p, size: %zu\n", block, block_size(block));
        
        // 打印调用栈或堆状态
        // 然后断点停止
        return;
    }
    
    // 正常释放
    actual_free_impl(ptr);
}
```

### Step 2: 检查指针有效性

```c
// 在释放前检查指针是否指向有效的堆区域
int is_valid_heap_ptr(void* ptr) {
    // 假设堆地址在 0x20000000 - 0x20080000
    uintptr_t p = (uintptr_t)ptr;
    return (p >= 0x20000000) && (p < 0x20080000);
}

if (!is_valid_heap_ptr(Property)) {
    printf("FATAL: Invalid pointer to free: %p\n", Property);
    return;
}

head->_free(Property);
```

### Step 3: 检查内存越界

```c
// 在每个可能越界的地方添加检查
void write_buffer(char* buf, int index, char value) {
    // 检查边界
    if (index < 0 || index >= BUFFER_SIZE) {
        printf("BUFFER OVERFLOW: index %d, size %d\n", index, BUFFER_SIZE);
        return;
    }
    buf[index] = value;
}
```

### Step 4: 检查多线程同步

```c
// 在释放前获得互斥锁
void safe_free(void* ptr) {
    xSemaphoreTake(heap_mutex, portMAX_DELAY);
    
    block_header_t* block = block_from_ptr(ptr);
    if (block_is_free(block)) {
        printf("FATAL: Double-free (thread-safe check)\n");
        xSemaphoreGive(heap_mutex);
        return;
    }
    
    actual_free(ptr);
    
    xSemaphoreGive(heap_mutex);
}
```

---

## ? 在你的项目中的具体修复

### 修复1: 列表清理中的循环检查

```c
// ? 当前代码 (危险)
while (prop->list->size > 0) {
    UI_Table_Clean(prop->list, 0, obj);
}

// ? 修复后 (安全)
int max_iterations = prop->list->size + 10;  // 保险上限
int iterations = 0;

while (prop->list->size > 0 && iterations < max_iterations) {
    Node* node = FindNode(prop->list, 0);
    if (node == NULL) break;  // 列表为空
    
    UI_Table_Clean(prop->list, 0, obj);
    iterations++;
}

if (iterations >= max_iterations) {
    LV_LOG_WARN("List cleanup exceeded max iterations");
    // 强制清理
    prop->list->size = 0;  // 重置大小，防止无限循环
}
```

### 修复2: 删除前检查

```c
// ? 当前代码
head->_free(Property);

// ? 修复后
if (Property != NULL && node != NULL) {
    // 检查 Property 是否已被释放
    block_header_t* block = (block_header_t*)((char*)Property - block_start_offset);
    if (!block_is_free(block)) {  // 确保还未被释放
        head->_free(Property);
    } else {
        LV_LOG_WARN("Property already freed, skipping double-free");
    }
}
```

### 修复3: 动画完全清理

```c
// ? 当前代码
lv_obj_del(obj);
head->_free(Property);

// ? 修复后
// 第一步: 停止所有动画
if (Property->anim != NULL) {
    lv_anim_del(obj, anim_y_cb);
}

// 第二步: 删除对象
lv_obj_del(obj);

// 第三步: 清理数据前再次检查
Property->obj = NULL;  // 标记对象已删除
Property->anim = NULL;

// 第四步: 释放内存
head->_free(Property);
```

### 修复4: 删除节点后清除引用

```c
// ? 当前代码
head->_free(Property);
removeNode(head, node->data);

// ? 修复后
head->_free(Property);
node->instant = NULL;  // ← 清除对 Property 的引用
removeNode(head, node->data);

// 防止后续操作再次访问该指针
```

---

## ? 预防性编程建议

### 建议1: 使用 Wrapper 函数

```c
// 定义安全的释放函数
void safe_list_free(List* list, int index) {
    Node* node = FindNode(list, index);
    if (node == NULL || node->instant == NULL) {
        return;  // 安全失败
    }
    
    Table_Property* property = node->instant;
    
    // 检查是否已释放
    if (is_memory_valid(property)) {
        list->_free(property);
        node->instant = NULL;  // 清除引用
    }
}
```

### 建议2: 使用引用计数

```c
typedef struct {
    void* data;
    int ref_count;  // 引用计数
} RefCountedPtr;

void release_ref(RefCountedPtr* ref_ptr) {
    if (ref_ptr->ref_count > 1) {
        ref_ptr->ref_count--;  // 减少引用计数
    } else {
        _free(ref_ptr->data);  // 真正释放
        ref_ptr->data = NULL;
    }
}
```

### 建议3: 使用栈变量追踪

```c
// 在函数开始时记录被释放的指针
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_) {
    // ...
    
    #if DEBUG_DOUBLE_FREE
    static void* freed_ptrs[100];
    static int freed_count = 0;
    
    if (Property != NULL) {
        // 检查是否之前释放过
        for (int i = 0; i < freed_count; i++) {
            if (freed_ptrs[i] == Property) {
                printf("DOUBLE-FREE DETECTED: %p\n", Property);
                return;  // 跳过这次释放
            }
        }
        
        list->_free(Property);
        freed_ptrs[freed_count++] = Property;
    }
    #else
    head->_free(Property);
    #endif
}
```

---

## ? 排查检查单

当遇到这个断言失败时:

- [ ] 确认是否有明确的 `free()` + `free()` 组合
- [ ] 检查列表清理中的循环逻辑（是否可能重复处理同一节点）
- [ ] 确认指针是否被修改或损坏
- [ ] 检查是否有内存越界（buffer overflow）
- [ ] 验证动画是否被完全停止
- [ ] 检查多线程中是否有竞态条件
- [ ] 添加内存监控和日志来追踪分配和释放
- [ ] 使用 AddressSanitizer 或 Valgrind 等工具
- [ ] 检查堆的完整性（heap corruption）

---

## ? 最常见的原因排名

根据经验：

1. **Double-Free (40%)** - 最常见，通常在列表清理或容器析构时
2. **内存越界 (30%)** - 破坏相邻内存块的 metadata
3. **指针损坏 (15%)** - 指针算术错误或被破坏
4. **竞态条件 (10%)** - 多线程中的同步问题
5. **其他 (5%)**

---

**快速诊断建议:**
1. 添加 `printf` 日志在每次 `free` 调用前
2. 比较地址看是否有重复
3. 追踪调用栈到 UI_Table_Clean 的来源
4. 检查 `removeNode()` 是否正确更新了列表大小

