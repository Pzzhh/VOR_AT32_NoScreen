# UI_ListBox.c 内存越界修复优先级清单

## ? P0 级（立即崩溃风险）

### P0.1 - 对象删除后动画继续运行

**位置**: Line 80 + Line 14-32

**问题**:
```c
// UI_Table_Clean() L80
lv_obj_del(obj);  

// 但这时 Property->anim 仍在运行！
// custom_anim_path() 会持续被回调:

// L14-32
static int32_t custom_anim_path(const lv_anim_t* a) {
    Table_Property* table = a->user_data;  // ← 已释放的指针！
    table->set_pos = table->set_clearence * (*table->ID);  // ← 越界访问链
    table->pos_y += (table->set_pos - table->pos_y) * 0.14;
    return (int32_t)table->pos_y;
}
```

**内存越界原因**:
- `obj` 被 `lv_obj_del()` 释放
- 但 `a->user_data` 指向 `Property`，仍被访问
- `Property` 本身也会在 L87 被释放
- 删除后仍被访问 = **越界**

**症状**: 
- 段错误 (SIGSEGV)
- 或返回垃圾的 `pos_y` 值

**修复必要性**: ? **必须，否则必定崩溃**

**修复步骤**:
1. 在 L80 之前调用 `lv_anim_del(obj, anim_y_cb)` 停止动画
2. 确保没有其他代码在动画中引用 `Property`

---

### P0.2 - Use-After-Free 日志访问

**位置**: Line 87 释放 + Line 90 访问

**问题**:
```c
head->_free(Property);  // L87: Property 指针被释放

// L90: 释放后仍访问
LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
                                        ^^^^^^^^^
                           这是已释放内存的访问！
```

**内存越界原因**:
- `Property` 指向堆上已分配的内存块
- `_free()` 将该内存块返还给堆管理器
- 随后访问 `Property->obj` 读取已释放的内存

**症状**:
- 日志显示垃圾地址值
- 或触发内存访问异常
- 数据损坏

**修复必要性**: ? **必须**

**修复步骤**:
```c
LV_LOG_USER("\r\nremove Index addr %x", Property->obj);  // 移到 L87 之前
head->_free(Property);
```

---

### P0.3 - NULL 指针越界取址

**位置**: Line 304

**问题**:
```c
lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, 
                          void* updata_cd, uint16_t insert_index)
{
    // ...
    Node* newnode = 0;
    
    if (insert_index == 0xffff) {
        newnode = add(head, table_proprity);  // 可能返回 NULL
    } else {
        newnode = insert(head, insert_index, table_proprity);
        if (newnode == 0)
            newnode = add(head, table_proprity);  // 仍可能返回 NULL
    }
    
    // ? 没有检查 newnode 是否为 NULL
    table_proprity->ID = &newnode->data;  // 如果 newnode==NULL，这是越界！
    // NULL->data 被翻译为 address 0x04（假设 data 在 offset 4）
}
```

**内存越界原因**:
- `newnode == NULL`
- `NULL->data` = 访问地址 `0x00000004` (假设)
- 这个地址不属于任何合法分配的内存

**症状**:
- 立即段错误 (SIGSEGV)

**修复必要性**: ? **必须**

**修复步骤**:
```c
if (insert_index == 0xffff) {
    newnode = add(head, table_proprity);
} else {
    newnode = insert(head, insert_index, table_proprity);
    if (newnode == 0) {
        newnode = add(head, table_proprity);
    }
}

// 添加检查
if (newnode == NULL) {
    lv_mem_free(table_proprity);
    return NULL;  // 错误处理
}
```

---

### P0.4 - 事件处理中的级联野指针

**位置**: Line 175-178

**问题**:
```c
static void Tabels_Touch_handler(lv_event_t* e)
{
    lv_obj_t* table = e->current_target;  // ← 可能已被删除
    lv_obj_t* table_container = lv_obj_get_parent(table);  // ← 可能返回 NULL
    Table_Contain_Property* table_Contain_Property = table_container->user_data;
    //                                                ^^^^^^^^^^^^^^
    //                         NULL->user_data = 越界访问！
    
    Table_Property* table_Property = table->user_data;  // ← 可能的野指针
}
```

**内存越界原因**:
- `table` 可能指向已被 `lv_obj_del()` 删除的对象
- `lv_obj_get_parent(table)` 可能返回 `NULL`
- `NULL->user_data` = 访问非法内存
- `table->user_data` 也可能指向已释放的 `Property`

**症状**:
- 段错误 (NULL 指针解引用)
- 或访问垃圾数据

**修复必要性**: ? **必须**

**修复步骤**:
```c
static void Tabels_Touch_handler(lv_event_t* e)
{
    lv_obj_t* table = e->current_target;
    if (table == NULL) return;  // 安全检查
    
    lv_obj_t* table_container = lv_obj_get_parent(table);
    if (table_container == NULL) return;  // 安全检查
    
    Table_Contain_Property* table_Contain_Property = 
        (Table_Contain_Property*)table_container->user_data;
    if (table_Contain_Property == NULL) return;  // 安全检查
    
    // ...
}
```

---

## ? P1 级（高概率崩溃/数据损坏）

### P1.1 - 双重野指针访问

**位置**: Line 186-194

**问题**:
```c
Table_Property* table_Property = table->user_data;  // ← 可能野指针

if (code == LV_EVENT_CLICKED) {
    table_Contain_Property->hightLight_index = *table_Property->ID;
    //                                          ^^^^^^^^^^^^^^^^
    //      table_Property 可能是野指针 → 访问越界
    //      table_Property->ID 也可能是野指针 → 再次越界
    //      *ID 访问双重野指针指向的内存 → 三重越界
}
```

**内存越界原因**:
- 如果 `table` 已被删除，`table->user_data` 指向已释放的 `Property`
- 访问 `table_Property->ID` 访问已释放内存中的指针字段
- 访问 `*ID` 访问该指针指向的内存 (可能任何地方)

**症状**:
- 段错误
- 数据损坏
- 随机崩溃

**修复必要性**: ? **必须（依赖 P0.4）**

---

### P1.2 - 容器删除时的清理混乱

**位置**: Line 230-240

**问题**:
```c
static void listbox_container_event_handler(lv_event_t* e)
{
    if (code == LV_EVENT_DELETE) {
        Table_Contain_Property* prop = (Table_Contain_Property*)lv_obj_get_user_data(obj);
        if (prop) {
            // ? 如果 size 更新失败，无限循环
            while (prop->list->size > 0) {
                UI_Table_Clean(prop->list, 0, obj);
            }
            
            // ? 释放后，其他代码可能仍访问 prop->list
            lv_mem_free(prop->list);  
            lv_mem_free(prop);
            // 如果中间有错误分支，可能重复释放
        }
    }
}
```

**内存越界原因**:
- 清理列表后释放 `prop->list`，但其他代码仍可能访问
- 如果异常中断，可能造成 Double-Free
- 无限循环可能导致栈溢出

**症状**:
- 无限循环 → 栈溢出 → 段错误
- Double-Free 检测
- 内存损坏

**修复必要性**: ? **高**

---

## ? P2 级（潜在问题）

### P2.1 - 动画参数验证缺失

**位置**: Line 16-32

**问题**:
```c
static int32_t custom_anim_path(const lv_anim_t* a) {
    static float movement_factor = 0.14;
    Table_Property* table = a->user_data;  // ← 没有验证这个指针
    
    if (table->isremove) {  // ← 假设 table 非 NULL
        return 0;
    }
    
    if (table->updata_cd != 0) {
        table->updata_cd(table);  // ← 函数指针可能垃圾值
    }
    
    if (table->set_pos > 1000 || table->set_clearence > 1000) {
        return 0;
    }
    
    // ← 这里可能访问越界
    table->set_pos = table->set_clearence * (*table->ID);
    //                                         ^^^^^^^
    //                                   可能指向无效内存
}
```

**内存越界原因**:
- `a->user_data` 没有 NULL 检查
- 如果 `user_data` 是垃圾值，立即越界
- 访问 `table->isremove` = 越界
- 调用 `table->updata_cd` = 调用垃圾地址

**修复必要性**: ? **必须（防御性编程）**

**修复步骤**:
```c
static int32_t custom_anim_path(const lv_anim_t* a) {
    if (a == NULL || a->user_data == NULL) return 0;  // 防御检查
    
    Table_Property* table = (Table_Property*)a->user_data;
    
    // 验证结构体合法性
    if (table->obj == NULL) return 0;  // obj 可能已删除
    
    // ...
}
```

---

### P2.2 - 列表查找中的越界

**位置**: Line 99-116

**问题**:
```c
uint8_t UI_Table_Find_Obj(List* head, lv_obj_t* obj, int* index)
{
    int counter = 0;
    Node* node = 0;
    while (1) {
        node = FindNode(head, counter++);  // ← 可能返回 NULL
        
        if (node->next == NULL)  // ← node 是 NULL 时，这是越界！
            return 0;
        
        if (node->instant != 0) {
            Table_Property* ins = node->instant;
            if (ins->obj == obj) {
                *index = counter;
                return 1;
            }
        }
    }
}
```

**内存越界原因**:
- `FindNode()` 返回 `NULL`
- 访问 `NULL->next` = 越界访问
- 永远不会找到对象，导致无限循环

**修复必要性**: ? **高（可能无限循环）**

---

## ? 修复优先顺序

```
优先级         问题              修复时间    风险       依赖关系
────────────────────────────────────────────────────────────────
P0.1 ┌── 动画继续运行      5分钟    立即崩溃  ← 必须先修
     │
P0.2 ├── Use-After-Free    3分钟    立即崩溃  无
     │
P0.3 ├── NULL 越界         5分钟    立即崩溃  无
     │
P0.4 ├── 事件链野指针      10分钟   立即崩溃  无
     │
P1.1 ├── 双重野指针        5分钟    数据损坏  ← 依赖 P0.4
     │
P1.2 ├── 清理混乱          15分钟   无限循环  无
     │
P2.1 └── 参数验证          10分钟   防御      无
        P2.2 列表查找        5分钟   防御      无

总耗时: ~58 分钟
风险等级: ? 严重 (6/8 问题为立即崩溃级别)
```

---

## ? 快速检查清单

在修复时，逐一验证:

- [ ] P0.1: `lv_anim_del()` 在 `lv_obj_del()` 之前调用
- [ ] P0.2: 日志语句在 `_free()` 之前
- [ ] P0.3: `newnode` 被检查为非 NULL
- [ ] P0.4: `table`, `table_container`, `user_data` 都被检查
- [ ] P1.1: 依赖 P0.4 的检查
- [ ] P1.2: 清理循环有次数限制或循环条件检查
- [ ] P2.1: `a->user_data` 被验证为非 NULL
- [ ] P2.2: `FindNode()` 返回值被检查

---

## ? 测试场景

修复后需要测试:

1. **快速删除**: 创建 → 立即删除，检查是否崩溃
2. **并发删除**: 多个表项同时删除，检查是否越界
3. **容器删除**: 删除整个容器，检查清理是否完整
4. **事件重放**: 删除后发送事件，检查是否处理
5. **边界值**: 插入失败场景，检查错误处理

