# 内存越界访问详细流程图

## 场景1: 动画继续访问已删除对象 (P0.1)

```
┌──────────────────────────────────────────────────────────────────┐
│                      创建阶段 (T=0~T1)                           │
├──────────────────────────────────────────────────────────────────┤

UI_Table_Create() 调用流程:
│
├─ 1. lv_mem_alloc(Table_Property)  ← 分配 Property
│       │
│       └─> 堆内存: [Property obj ptr | ID ptr | anim ptr | ... ]
│
├─ 2. lv_anim_init(&a)              ← 初始化动画结构
│       │
│       └─> a.user_data = Property  ← 关键: 动画持有 Property 指针
│
├─ 3. lv_anim_start(&a)             ← 启动动画
│       │
│       ├─> 注册回调: anim_y_cb()
│       ├─> 注册路径: custom_anim_path()  ← 会反复调用
│       └─> 每一帧调用: custom_anim_path(anim)
│
└─> 现在: 动画每帧都会调用:
        custom_anim_path(&a)
        {
            Table_Property* table = a->user_data;  ← Property
            return (int32_t)table->pos_y;          ← 访问 Property
        }

时间流逝: 动画持续运行
│
├─ T1 = 100ms 后  
│ ├─ Animation running (第1帧)  ? custom_anim_path() 正常
│ ├─ Animation running (第2帧)  ? custom_anim_path() 正常
│ └─ ...
│
└─> 现在一切正常，Property 有效，动画运行中

┌──────────────────────────────────────────────────────────────────┐
│                    删除阶段 (T2 删除操作)                         │
├──────────────────────────────────────────────────────────────────┤

UI_Table_Clean() 调用流程:
│
├─ 1. Table_Property* Property = node->instant;  ← 获取 Property
│      │
│      └─> Property 仍然有效
│
├─ 2. lv_obj_t* obj = Property->obj;
│      │
│      └─> obj 仍然有效
│
├─ 3. lv_anim_del(obj, anim_y_cb);  ← ?? 问题: 这可能删不掉!
│      │
│      ├─> custom_anim_path 的动画是通过 lv_anim_set_path_cb() 设置的
│      ├─> lv_anim_del(obj, anim_y_cb) 只删除 exec_cb 为 anim_y_cb 的动画
│      └─> 但 custom_anim_path 仍然可能在运行！?
│
├─ 4. lv_obj_del(obj);  ← ? 删除 LVGL 对象
│      │
│      └─> 堆内存: [已删除，被标记为可用]
│            obj 指针现在悬空
│
├─ 5. head->_free(Property);  ← ? 释放 Property
│      │
│      └─> 堆内存: [已释放，被标记为可用]
│            Property 指针现在悬空
│
└─> 现在: Property 和 obj 都已删除

┌──────────────────────────────────────────────────────────────────┐
│                   越界访问阶段 (T3 动画继续运行)                  │
├──────────────────────────────────────────────────────────────────┤

动画还在运行！(例如在 FreeRTOS 中断或其他线程)

│
├─ T3 = 100ms 后 (deleted 后 20ms)
│ ├─ Animation running (第100帧)
│ │   custom_anim_path(&a) 被调用
│ │   {
│ │       Table_Property* table = a->user_data;
│ │       ↓
│ │       访问 0xDEADBEEF (垃圾值，因为内存已释放)
│ │
│ │       if (table->isremove)  ← ? MEMORY OVERFLOW!
│ │       {                        读取已释放的内存
│ │           return 0;            可能读到: 任意值
│ │       }                        可能的结果: 
│ │                                 - 随机跳转
│ │       table->updata_cd(table)  - 调用无效函数指针
│ │           ↑                     - 段错误
│ │       ? INVALID FUNCTION CALL  - 数据损坏
│ │
│ │       table->set_pos = table->set_clearence * (*table->ID);
│ │       ↑          ↑        ↑              ↑
│ │       全是已释放的内存的访问！
│ │
│ │       return (int32_t)table->pos_y;
│ │       返回垃圾值
│ │   }
│ │
│ └─> ? CRASH: 段错误或数据损坏
│
└─> Animation 仍在继续...直到崩溃


┌──────────────────────────────────────────────────────────────────┐
│                        内存状态时间线                             │
├──────────────────────────────────────────────────────────────────┤

时间      heap state              custom_anim_path 访问
─────────────────────────────────────────────────────────
T0      ┌─────────────────┐
(创建)   │ Property ?      │      table = ? 有效
        │ - isremove      │      table->isremove ?
        │ - set_pos       │      table->set_clearence ?
        │ - pos_y         │      table->ID ?
        └─────────────────┘
        
        动画正在运行: table 有效
        
T1                              ? 访问成功
(100ms)  ┌─────────────────┐
        │ Property ?      │      table = ? 有效
        └─────────────────┘
        
        动画继续: table 有效
        
T2      ┌─────────────────┐
(删除)   │[已释放区域 ?] │      ← 内存被释放
        └─────────────────┘
        
        Property 已删除
        obj 已删除
        
T3      ┌─────────────────┐
(120ms) │[堆碎片/其他数据]│      ? table = 垃圾值
        │                 │      ? table->isremove = ???
        │[可能被其他分配] │      ? table->set_clearence = ???
        └─────────────────┘      ? table->pos_y = ???
        
        动画继续调用 custom_anim_path()
        ? OVERFLOW: 访问堆碎片或其他数据的内存


┌──────────────────────────────────────────────────────────────────┐
│                      修复方案                                     │
├──────────────────────────────────────────────────────────────────┤

当前错误的顺序:
    lv_obj_del(obj);           // obj 删除，但动画仍运行
    head->_free(Property);     // Property 删除，但动画仍访问

正确的顺序:
    lv_anim_del(obj, anim_y_cb);     // 确保动画停止
    lv_obj_del(obj);                 // 然后删除对象
    head->_free(Property);           // 最后释放结构体

原因:
    - 动画持有 Property 指针 (a->user_data = Property)
    - 动画的执行回调会访问这个指针
    - 必须在删除 Property 前停止动画
    - 必须在删除对象前停止所有动画
```

---

## 场景2: Use-After-Free 在日志中 (P0.2)

```
┌──────────────────────────────────────────────────────────────────┐
│                      当前代码 (错误)                              │
├──────────────────────────────────────────────────────────────────┤

void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ... 删除代码 ...
    
    head->_free(Property);              // L87: 释放内存
    // ↑
    // heap manager 标记该内存为"可用"
    // Property 指针现在悬空
    
    removeNode(head, node->data);
    ID_UPdata(head);
    
    // ? L90: Use-After-Free
    LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
    //                                      ^^^^^^^^^
    // 这访问已释放的内存！
    //
    // 时序:
    // 1. heap_free() 被调用
    // 2. Property 指向的内存块被返回给 heap 管理器
    // 3. 该内存块可能被立即重新分配给其他对象
    // 4. 访问 Property->obj 现在读取的是新对象的数据
    //
    // 症状:
    // - 日志打印垃圾地址值
    // - 或触发内存访问违规异常


┌──────────────────────────────────────────────────────────────────┐
│                    内存状态时间线                                 │
├──────────────────────────────────────────────────────────────────┤

释放前:
    heap memory:
    ┌─────────────────────┐
    │ Property (index 5)  │  ← Property 指针指向这里
    │ - obj = 0x20001234  │  ← obj 字段保存对象地址
    │ - ID = 0x20002345   │  ← ID 字段保存 int* 地址
    │ - style = ...       │
    └─────────────────────┘

释放时 (L87):
    head->_free(Property)
    ↓
    heap manager:
    该内存块被标记为"可用"
    可能随时被其他分配覆盖

访问时 (L90):
    LV_LOG_USER("\r\nremove Index addr %x", Property->obj)
    ↓
    读取 Property->obj
    ↓
    可能得到:
    a) 0xDEADBEEF (填充字节)
    b) 其他对象的数据 (内存被重新分配)
    c) 垃圾值 (内存未初始化)
    
    任何情况都不对！


┌──────────────────────────────────────────────────────────────────┐
│                      修复方案                                     │
├──────────────────────────────────────────────────────────────────┤

正确的顺序:

void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ...
    
    // 第一步: 日志记录 (在释放前)
    LV_LOG_USER("\r\nremove Index addr %x", Property->obj);  // ? 有效
    
    // 第二步: 删除和释放
    removeNode(head, node->data);
    ID_UPdata(head);
    head->_free(Property);  // 现在释放是安全的
}

原因:
    - 访问必须在内存释放前完成
    - 一旦释放，指针就不再有效
    - 遵守 RAII 原则: 访问后释放
```

---

## 场景3: NULL 指针越界取址 (P0.3)

```
┌──────────────────────────────────────────────────────────────────┐
│                     问题代码                                      │
├──────────────────────────────────────────────────────────────────┤

lv_obj_t* UI_Table_Create(...)
{
    Node* newnode = 0;
    
    if (insert_index == 0xffff) {
        newnode = add(head, table_proprity);
    } else {
        newnode = insert(head, insert_index, table_proprity);
        if (newnode == 0)
            newnode = add(head, table_proprity);  // ← 可能仍返回 NULL
    }
    
    // ? 没有检查 newnode
    table_proprity->ID = &newnode->data;  // NULL->data = 越界!
}


┌──────────────────────────────────────────────────────────────────┐
│                   NULL 指针越界取址的含义                         │
├──────────────────────────────────────────────────────────────────┤

假设 Node 结构体:
    typedef struct Node {
        int data;           // offset 0, size 4
        void* instant;      // offset 4, size 4
        struct Node* next;  // offset 8, size 4
    } Node;

正常的取址:
    Node* node = malloc(sizeof(Node));
    
    node 的值:     0x20001000 (有效的堆地址)
    &node->data:   0x20001000 + 0 = 0x20001000
    &node->next:   0x20001000 + 8 = 0x20001008

NULL 指针的取址:
    newnode = NULL;
    
    newnode 的值:     0x00000000 (NULL)
    &NULL->data:      0x00000000 + 0 = 0x00000000  ← 无效地址!
    &newnode->data:   0x00000000 + 4 = 0x00000004  ← 无效地址!
    
    访问这些地址的内存 = 段错误


┌──────────────────────────────────────────────────────────────────┐
│                   为什么会立即崩溃                                │
├──────────────────────────────────────────────────────────────────┤

步骤 1: newnode = NULL
        ↓
步骤 2: table_proprity->ID = &newnode->data;
        
        计算地址:
        newnode = 0x00000000 (NULL)
        &newnode->data = 0x00000000 + 4 = 0x00000004
        ↓
步骤 3: 赋值操作试图获取 newnode->data 的地址
        
        CPU 指令类似:
        mov eax, [0x00000004]   ; 读取地址 0x4 的内存
        ↓
        地址 0x4 不在有效的内存映射中
        ↓
        MMU 触发 Page Fault
        ↓
        OS 抛出异常
        ↓
        进程接收 SIGSEGV 信号
        ↓
        ? 段错误，程序崩溃


┌──────────────────────────────────────────────────────────────────┐
│                      修复方案                                     │
├──────────────────────────────────────────────────────────────────┤

添加 NULL 检查:

if (newnode == NULL) {
    // 内存分配失败
    lv_mem_free(table_proprity);  // 清理已分配的 table_proprity
    return NULL;  // 向调用者报告错误
}

// 现在 newnode 肯定非 NULL
table_proprity->ID = &newnode->data;  // ? 安全的取址
```

---

## 场景4: 事件处理中的级联野指针 (P0.4)

```
┌──────────────────────────────────────────────────────────────────┐
│              事件处理野指针链的形成                               │
├──────────────────────────────────────────────────────────────────┤

创建阶段:
    UI_Table_Create()
    {
        lv_obj_t* table = lv_table_create(parent);  // 创建对象
        table->user_data = table_proprity;           // 绑定数据
        lv_obj_add_event_cb(table, Tabels_Touch_handler, ...);
        //                   ↑
        //          注册事件处理函数，保存 table 引用
    }

删除阶段:
    UI_Table_Clean()
    {
        lv_obj_del(obj);  // table 被删除
        // LVGL 标记该对象为已删除
        // 但事件处理函数的指针仍然存在！
    }

触发崩溃:
    用户输入 / 系统事件
    {
        Tabels_Touch_handler 被调用
        {
            lv_obj_t* table = e->current_target;  // ← 指向已删除的对象!
            // table 的值是一个不再有效的地址
            // 它可能:
            //   1. 指向已释放的堆内存
            //   2. 被 LVGL 标记为已删除
            //   3. 内存内容是垃圾值
            
            lv_obj_t* table_container = lv_obj_get_parent(table);
            // 尝试访问已删除对象的 parent 指针
            // 这可能返回垃圾值或 NULL
            
            Table_Contain_Property* table_Contain_Property = 
                table_container->user_data;
            // 解引用可能的 NULL 或垃圾指针
            // ? 内存越界!
            
            Table_Property* table_Property = table->user_data;
            // 再次访问已删除对象
            // ? 内存越界!
            
            *table_Property->ID = ...;
            // 访问可能已释放的结构体的字段
            // ? 三重越界!
        }
    }


┌──────────────────────────────────────────────────────────────────┐
│                   内存和事件时间线                                │
├──────────────────────────────────────────────────────────────────┤

时间    对象状态              事件处理状态
────────────────────────────────────────────
T0      table 创建 ?          处理函数已注册
(创建)   obj->user_data = ptr  
        
T1      table 运行中 ?        触发事件 → 处理函数调用
(运行)   可以安全访问         可以安全访问所有指针
        
T2      lv_obj_del(table)    （同时或之后）
(删除)   table 内存被释放      处理函数指针仍存在！
        obj->user_data = ???   但指向已删除的对象
        
T3      ? 内存区域不再有效   触发延迟事件或系统事件
(危险)   ? 可能被其他对象使用  处理函数被调用！
                               ↓
                               访问已删除的 table
                               ↓
                               ? 崩溃


┌──────────────────────────────────────────────────────────────────┐
│                    为什么会发生延迟调用                           │
├──────────────────────────────────────────────────────────────────┤

场景A: FreeRTOS 多任务
    任务1: 删除表项 (UI_Table_Clean)
           lv_obj_del(table)  ← 立即执行
    
    任务2: 处理触屏事件 (延迟)
           触屏中断发生 → 队列消息 → 处理延迟
           当处理时，table 已被删除！
    
场景B: LVGL 事件队列
    事件被注册时:
        e->current_target = table
    
    事件延迟处理 (例如 post 到事件队列):
        稍后处理事件
        但 table 已被删除
    
场景C: 递归事件
    Tabels_Touch_handler 被调用
    ↓
    尝试访问 table_Property
    ↓
    但 Property 的析构函数可能触发其他事件
    ↓
    其他代码删除了 table
    ↓
    继续访问 table_Property
    ↓
    ? 使用被删除的对象


┌──────────────────────────────────────────────────────────────────┐
│                      修复方案                                     │
├──────────────────────────────────────────────────────────────────┤

添加防御性检查:

static void Tabels_Touch_handler(lv_event_t* e)
{
    // 第一层: 检查直接指针
    lv_obj_t* table = e->current_target;
    if (table == NULL || !lv_obj_is_valid(table)) return;  // ? 防御
    
    // 第二层: 检查父容器
    lv_obj_t* table_container = lv_obj_get_parent(table);
    if (table_container == NULL || !lv_obj_is_valid(table_container)) 
        return;  // ? 防御
    
    // 第三层: 检查 user_data 指针
    Table_Contain_Property* table_Contain_Property = 
        (Table_Contain_Property*)table_container->user_data;
    if (table_Contain_Property == NULL) return;  // ? 防御
    
    // 第四层: 检查子对象的 user_data
    Table_Property* table_Property = table->user_data;
    if (table_Property == NULL) return;  // ? 防御
    
    // 第五层: 检查嵌套指针
    if (table_Property->ID == NULL) return;  // ? 防御
    
    // 现在可以安全地使用指针链
    lv_event_code_t code = e->code;
    if (code == LV_EVENT_CLICKED) {
        table_Contain_Property->hightLight_index = *table_Property->ID;  // ? 安全
    }
}

或者，使用对象有效性验证 (如果 LVGL 支持):
    if (!lv_obj_is_valid(table)) {
        // 对象已被删除或无效
        return;
    }
```

---

## 场景5: 容器删除时的双重释放 (P1.2)

```
┌──────────────────────────────────────────────────────────────────┐
│                    当前清理代码的问题                             │
├──────────────────────────────────────────────────────────────────┤

static void listbox_container_event_handler(lv_event_t* e)
{
    if (code == LV_EVENT_DELETE) {
        Table_Contain_Property* prop = ...;
        
        // ? 问题1: 无限循环风险
        while (prop->list->size > 0) {
            UI_Table_Clean(prop->list, 0, obj);
        }
        // 如果 size 更新失败，永远不会退出
        
        // ? 问题2: 清理后释放，但其他代码可能仍在清理
        lv_mem_free(prop->list);
        lv_mem_free(prop);
    }
}


┌──────────────────────────────────────────────────────────────────┐
│                   无限循环场景                                    │
├──────────────────────────────────────────────────────────────────┤

初始状态:
    prop->list->size = 3
    prop->list->head = [Node0] → [Node1] → [Node2] → NULL

迭代 1:
    UI_Table_Clean(list, 0, obj)
    └─ 假设: 删除 Node0
    └─ 预期: removeNode() 更新 size = 2
    └─ 实际: removeNode() 失败或没有正确更新
    └─ 结果: size 仍然是 3 ?
    
    while 条件检查: size (3) > 0 → 继续循环

迭代 2:
    UI_Table_Clean(list, 0, obj)
    └─ Node0 已删除，现在删除 Node1
    └─ 预期: size = 2
    └─ 实际: size = 3 (未更新) ?
    
    while 条件检查: size (3) > 0 → 继续循环

...

迭代 N (假设 N = 无穷):
    堆栈持续增长
    └─ UI_Table_Clean 的栈帧不断叠加
    └─ 局部变量占用堆栈空间
    └─ 堆栈溢出
    └─ ? Stack Overflow


┌──────────────────────────────────────────────────────────────────┐
│                      修复方案                                     │
├──────────────────────────────────────────────────────────────────┤

方案1: 添加迭代次数限制 (保险)
    
    int max_iterations = 1000;  // 保险上限
    while (prop->list->size > 0 && max_iterations-- > 0) {
        UI_Table_Clean(prop->list, 0, obj);
    }
    if (max_iterations <= 0) {
        LV_LOG_WARN("ListBox cleanup timeout");
    }

方案2: 显式清理所有节点 (更好)
    
    // 获取所有节点索引并一次性删除
    int size = prop->list->size;
    for (int i = 0; i < size; i++) {
        if (prop->list->size > 0) {
            UI_Table_Clean(prop->list, 0, obj);
        }
    }

方案3: 实现安全的链表清理函数 (最好)
    
    void List_Clear(List* list) {
        while (list->head != NULL) {
            Node* temp = list->head;
            list->head = list->head->next;
            list->_free(temp->instant);
            list->_free(temp);
            list->size--;
        }
    }
    
    然后在删除时调用:
    List_Clear(prop->list);
    lv_mem_free(prop->list);
    lv_mem_free(prop);
```

---

## 总结: 越界访问的模式

所有越界访问都遵循这个模式:

```
步骤1: 获取指针到有效对象
    ptr = valid_object;  ? 这时有效

步骤2: 删除/释放对象
    free(ptr);  or  delete(ptr);
    对象的内存被标记为可用
    ptr 成为悬空指针 ?

步骤3: 继续使用指针
    ptr->member = ...;  ? 访问无效的内存
    *ptr = ...;         ? 访问无效的内存
    result = *ptr;      ? 读取已删除的内存

步骤4: 崩溃
    ? 段错误或数据损坏

防护原则:
    ┌─────────────────────────────────────┐
    │ "不删除，或在删除前停止所有访问"      │
    └─────────────────────────────────────┘
    
    - 停止所有动画 (P0.1)
    - 停止所有事件处理 (P0.4)
    - 停止所有回调 (P2.1)
    - 在删除前检查指针有效性 (P0.4)
    - 防御性编程: 总是检查指针 (P2.1, P2.2)
```

