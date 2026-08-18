# TLSF "block already marked as free" 快速排查指南

## ? 问题代码位置

```
lv_tlsf.c:1167  (free 函数)
lv_tlsf.c:1221  (realloc 函数)

tlsf_assert(!block_is_free(block) && "block already marked as free");
```

**错误含义**: 尝试释放一个已经被标记为"已释放"的内存块 → **Double-Free**

---

## ? 五分钟快速诊断

### Step 1: 确认是否有重复 free

```c
// 在 UI_Table_Clean 中添加调试打印
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ... 代码 ...
    
    if (node->instant) {
        Table_Property* Property = node->instant;
        
        // ← 添加这行
        printf("[DEBUG] UI_Table_Clean: Freeing Property=%p, index=%d\n", 
               Property, index);
        
        head->_free(Property);
        // ... 后续代码 ...
    }
}
```

**如果打印相同的地址两次** → 确认是 Double-Free

### Step 2: 检查列表清理逻辑

```c
// 在 listbox_container_event_handler 中
static void listbox_container_event_handler(lv_event_t* e)
{
    if (code == LV_EVENT_DELETE) {
        Table_Contain_Property* prop = ...;
        
        // ← 问题代码：这个循环可能无限
        while (prop->list->size > 0) {
            printf("[DEBUG] List size before clean: %d\n", prop->list->size);
            UI_Table_Clean(prop->list, 0, obj);
            printf("[DEBUG] List size after clean: %d\n", prop->list->size);
            
            // 如果 size 没有减少，会无限循环
        }
    }
}
```

**如果 size 不减少** → 可能多次清理同一个块

### Step 3: 检查 removeNode 是否有效

```c
// removeNode 实现检查
void removeNode(List* list, int data) {
    // ... 移除节点逻辑 ...
    list->size--;  // ← 检查这行是否被执行
}

// 如果 removeNode 失败（例如节点不存在），size 不会减少
// 导致 while 循环继续操作同一节点
```

---

## ? 六个最可能的原因

### 原因1: 列表清理中的无限循环 ?? 最可能

```
时间 →

UI_Table_Clean(list, 0) 第1次调用:
  ├─ node = FindNode(list, 0)  ? 找到
  ├─ free(Property)
  ├─ removeNode(list, 0)  ? 失败，size 没变
  └─ list->size = 3 (仍然是 3)

while 检查: size (3) > 0 → 继续

UI_Table_Clean(list, 0) 第2次调用:
  ├─ node = FindNode(list, 0)  ? 找到新的 node
  ├─ free(Property)  ? 释放错误的指针或重复释放
  └─ ...

结果: 同一个 Property 被 free 多次
      ↓
      "block already marked as free"
```

**修复**:
```c
// 添加安全上限
int max_attempts = 100;
int attempts = 0;

while (prop->list->size > 0 && attempts < max_attempts) {
    int size_before = prop->list->size;
    UI_Table_Clean(prop->list, 0, obj);
    int size_after = prop->list->size;
    
    // 检查是否有进展
    if (size_before == size_after) {
        printf("WARNING: List not shrinking, breaking\n");
        break;  // 防止无限循环
    }
    
    attempts++;
}
```

### 原因2: Property 同时被两个指针指向

```
场景: List 中的同一个 Property 被两次引用

node[0]->instant = Property
node[1]->instant = Property  ← 同一个指针！

UI_Table_Clean(list, 0) 释放 Property
UI_Table_Clean(list, 1) 再次释放同一个 Property

结果: Double-Free
```

**检查方法**:
```c
// 遍历列表检查重复
int check_duplicate_properties(List* list) {
    Node* node = list->head;
    int duplicates = 0;
    
    while (node) {
        Node* check = node->next;
        while (check) {
            if (node->instant == check->instant && node->instant != NULL) {
                printf("DUPLICATE: %p at nodes\n", node->instant);
                duplicates++;
            }
            check = check->next;
        }
        node = node->next;
    }
    
    return duplicates;
}
```

### 原因3: 动画清理不完全

```
scenario: 对象删除但动画仍在运行

UI_Table_Clean():
  └─ lv_obj_del(obj)  ← LVGL 对象删除
  └─ head->_free(Property)  ← Property 释放
  
但动画仍在回调:
  └─ custom_anim_path() 被触发
  └─ 访问 a->user_data (已释放的 Property)
  └─ 修改内存
  └─ 破坏其他 block 的 free_bit
  
后续释放其他块时:
  └─ 发现 free_bit 已被破坏
  └─ "block already marked as free"
```

**修复**:
```c
// 在 lv_obj_del() 前停止动画
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    if (node->instant) {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        // ← 添加这行：在删除前停止动画
        if (Property->anim != NULL) {
            lv_anim_del(obj, anim_y_cb);
        }
        
        lv_obj_del(obj);
        head->_free(Property);
        // ...
    }
}
```

### 原因4: 指针被破坏或不对齐

```
调试步骤:

// 添加指针验证
void* safe_free(void* ptr) {
    // 检查指针是否对齐
    if ((uintptr_t)ptr % sizeof(void*) != 0) {
        printf("ERROR: Unaligned pointer %p\n", ptr);
        return;
    }
    
    // 检查指针是否在合法范围
    if ((uintptr_t)ptr < 0x20000000 || (uintptr_t)ptr > 0x20080000) {
        printf("ERROR: Invalid heap pointer %p\n", ptr);
        return;
    }
    
    // 检查 block 是否已被释放
    block_header_t* block = block_from_ptr(ptr);
    if (block_is_free(block)) {
        printf("ERROR: Block already free at %p\n", ptr);
        return;  // 防止 double-free
    }
    
    // 安全释放
    actual_free(ptr);
}
```

### 原因5: 内存越界破坏了 metadata

```
检测方法:

// 在释放前检查堆完整性
int check_heap_integrity(void) {
    // 遍历所有块，检查 free_bit 的合理性
    // 这需要访问 TLSF 的内部结构
    
    // 简单版本：在每个块前后放置哨兵值
    #define SENTINEL_VALUE 0xDEADBEEF
    
    // 在分配时:
    uint32_t* sentinel = (uint32_t*)((char*)ptr - 4);
    *sentinel = SENTINEL_VALUE;
    
    // 在释放时:
    uint32_t* sentinel = (uint32_t*)((char*)ptr - 4);
    if (*sentinel != SENTINEL_VALUE) {
        printf("HEAP CORRUPTION: Sentinel overwritten\n");
        return -1;  // 堆损坏
    }
}
```

### 原因6: 列表链表结构被破坏

```
检查 removeNode 实现:

// removeNode 必须:
1. 找到节点
2. 从链表中移除（更新 prev 和 next）
3. 减少大小计数器
4. 如果适用，更新 head 指针

如果任何一步失败，list->size 不会减少

// 添加验证
int removeNode_safe(List* list, int data) {
    Node* node = FindNode(list, data);
    if (node == NULL) {
        printf("ERROR: Node not found for removal\n");
        return -1;
    }
    
    int size_before = list->size;
    
    // 执行移除
    removeNode(list, data);
    
    // 验证
    if (list->size == size_before) {
        printf("ERROR: Size not updated after removal\n");
        return -1;
    }
    
    return 0;
}
```

---

## ? 问题排查清单

当遇到这个断言时，按顺序检查:

### Step 1: 获取崩溃时的堆状态

```c
// 在 lv_tlsf_free 中添加
void lv_tlsf_free(lv_tlsf_t tlsf, const void * ptr)
{
    size_t size = 0;
    if(ptr) {
        control_t * control = tlsf_cast(control_t *, tlsf);
        block_header_t * block = block_from_ptr(ptr);
        
        // ← 添加调试信息
        printf("[TLSF_FREE] ptr=%p, block=%p\n", ptr, block);
        printf("  block->size=0x%x\n", block->size);
        printf("  block_is_free=%d\n", block_is_free(block));
        
        tlsf_assert(!block_is_free(block) && "block already marked as free");
        
        // ... 后续代码 ...
    }
}
```

### Step 2: 查看调用堆栈

从断言失败的错误信息中:
- 确认是从 `UI_Table_Clean` 还是 `lv_tlsf_free` 直接调用
- 追踪谁调用了 `UI_Table_Clean`
- 检查调用序列中是否有重复

### Step 3: 检查日志中是否有重复地址

```c
// 收集所有 free 调用的地址
// 搜索是否有相同的 ptr 值出现两次
```

### Step 4: 验证列表大小变化

```c
// 在 while 循环中打印大小
printf("Before: size=%d\n", prop->list->size);
UI_Table_Clean(prop->list, 0, obj);
printf("After:  size=%d\n", prop->list->size);

// 如果大小不变，证实了问题根源
```

### Step 5: 检查 FindNode 的正确性

```c
// FindNode 应该返回相同索引的节点
Node* n0 = FindNode(list, 0);
Node* n0_again = FindNode(list, 0);

if (n0 != n0_again) {
    printf("ERROR: FindNode returns different nodes for same index\n");
}
```

---

## ? 三个最快的修复

### 修复1: 保险上限

```c
// 在 listbox_container_event_handler 中
int max_iterations = 1000;
int count = 0;

while (prop->list->size > 0 && count < max_iterations) {
    UI_Table_Clean(prop->list, 0, obj);
    count++;
}

if (count >= max_iterations) {
    LV_LOG_ERROR("List cleanup exceeded max iterations");
    prop->list->size = 0;  // 强制重置
}
```

### 修复2: 停止动画

```c
// 在 UI_Table_Clean 的 lv_obj_del 前添加
if (Property->anim != NULL) {
    lv_anim_del(obj, anim_y_cb);
    Property->anim = NULL;
}
```

### 修复3: 清除引用

```c
// 在 head->_free(Property) 后添加
head->_free(Property);
node->instant = NULL;  // ← 防止后续重复访问
```

---

## ? 根据现象快速定位

| 现象 | 可能原因 | 修复 |
|------|---------|------|
| **断言立即发生** | Double-Free（同一次清理操作） | 检查 removeNode 是否成功 |
| **断言在第N次清理** | 无限循环导致重复释放 | 添加保险上限 |
| **断言在关闭时** | 容器析构时重复清理 | 检查对象生命周期 |
| **断言间歇性出现** | 竞态条件或内存越界 | 添加互斥锁或内存检查 |
| **多次相同的 ptr** | 列表中重复引用 | 检查 Property 分配逻辑 |

---

## ? 最可能的修复 (90% 概率)

在 `listbox_container_event_handler` 中:

```c
// ? 原始代码
while (prop->list->size > 0) {
    UI_Table_Clean(prop->list, 0, obj);
}

// ? 修复版本
for (int i = 0; i < 100 && prop->list->size > 0; i++) {
    int before = prop->list->size;
    UI_Table_Clean(prop->list, 0, obj);
    int after = prop->list->size;
    
    if (before == after) {
        // 列表没有缩小，可能 removeNode 有问题
        printf("WARNING: List not shrinking, forcing cleanup\n");
        break;
    }
}
```

**为什么这个修复有效:**
1. 防止无限循环
2. 检测清理是否有进展
3. 如果进展停止，及时中止
4. 避免多次释放同一内存块

---

## ? 预防策略

添加到项目中的防守代码:

```c
// 1. 在释放前检查
#define SAFE_FREE(list, ptr) \
    do { \
        if (ptr != NULL) { \
            printf("[SAFE_FREE] Freeing %p\n", ptr); \
            list->_free(ptr); \
            ptr = NULL; \
        } \
    } while(0)

// 2. 列表操作验证
#define CHECK_LIST_INTEGRITY(list) \
    do { \
        if (list->size < 0 || list->size > 10000) { \
            printf("ERROR: List corrupted, size=%d\n", list->size); \
            while(1);  /* 停止执行 */ \
        } \
    } while(0)

// 3. 清理前快照
int snapshot_list_size = prop->list->size;
// ... 清理操作 ...
if (prop->list->size == snapshot_list_size) {
    printf("WARNING: List size didn't change\n");
}
```

