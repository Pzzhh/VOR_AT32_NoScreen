# UI_ListBox.c 中的 TLSF Double-Free 具体修复方案

## ? 问题定位

**断言失败发生在**: 
- 位置: `lv_obj_set_local_style_prop()` → `get_local_style()` → `lv_mem_realloc()`
- 原因: 样式数据的生命周期管理不当

**根本原因**: UI_ListBox.c 中的样式对象嵌入在 Table_Property 结构体中，删除对象时没有先清理样式，导致 LVGL 内部的样式数组指向已释放的内存。

---

## ? 具体代码分析

### 问题代码 - UI_ListBox.c

```c
// 第 268-310 行
typedef struct {
    int data;
    Table_Property* instant;
    struct Node2* next;
} Node2;

typedef struct {
    Node2* head;
    int size;
    void(*_free)(void* _Block);
    void* (*_malloc)(size_t size);
} List2;

lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, 
                          void* updata_cd, uint16_t insert_index)
{
    lv_obj_t* parent = table_Contain_Property->obj;
    List* head = table_Contain_Property->list;
    lv_obj_t* table = lv_table_create(parent);

    Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));
    // ... node 操作 ...
    
    lv_style_init(&table_proprity->style);  // ← style 在堆分配的结构体内
    lv_style_set_outline_width(&table_proprity->style, 3);
    lv_style_set_outline_pad(&table_proprity->style, 3);
    lv_style_set_outline_color(&table_proprity->style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_font(&table_proprity->style, &lv_font_montserrat_14);
    
    lv_obj_add_style(table, &table_proprity->style, 0);  // ← 添加样式
    // ↑ 关键问题: table->styles 数组现在包含指向 table_proprity->style 的指针
    
    table->user_data = table_proprity;
    
    // ... 其他设置 ...
    
    return table;
}
```

**问题**:
- `table_proprity->style` 是一个嵌入在堆分配结构体内的对象
- 通过 `lv_obj_add_style()` 将其指针添加到 `table->styles` 数组
- 当 `Table_Property` 被 free 时，样式对象也被释放
- 但 `table->styles` 数组仍然指向已释放的样式对象

### 删除代码 - UI_ListBox.c 第 63-92 行

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    _head = (List2*)head;
    Node* node = FindNode(head, index);
    if (node == NULL)
        return;
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        lv_anim_del(obj, anim_y_cb);
        lv_style_reset(&Property->style);  // ← 重置样式
        
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        // ↑ 这行很重要，但位置不对（在 reset 之后）
        
        lv_obj_del(obj);  // ← 删除对象（此时 obj->styles 被释放）
        head->_free(Property);  // ← 释放 Property（包括 style）
        removeNode(head, node->data);
        ID_UPdata(head);

        LV_LOG_USER("\r\nremove Index addr %x", Property->obj);  // ? Use-After-Free
    }
}
```

**问题**:
1. `lv_style_reset()` 是对样式内容的重置，不是从对象中移除样式
2. `lv_obj_remove_style()` 在删除后，效果已经不重要
3. `lv_obj_del()` 删除对象，但对象的 `styles` 数组可能以某种方式仍被引用
4. `head->_free(Property)` 释放包含样式的结构体

**结果**:
```
时间线:

1. obj->styles = [指向 Property->style]
2. lv_obj_del(obj)  ← obj->styles 可能被释放，但引用仍存在
3. head->_free(Property)  ← Property->style 也被释放
4. 后续某处: lv_obj_set_local_style_prop(obj, ...)
   └─ get_local_style(obj, ...)
     └─ lv_mem_realloc(obj->styles, ...)  ← 对已释放的 styles 数组 realloc
     └─ TLSF: block_is_free() = true
     └─ tlsf_assert 失败!
```

---

## ? 修复方案

### 方案A: 正确的样式清理顺序 ? 最简单且最有效

**修改 UI_Table_Clean 函数**:

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    _head = (List2*)head;
    Node* node = FindNode(head, index);
    if (node == NULL)
        return;
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        // ? 第一步: 停止动画（在样式清理前）
        lv_anim_del(obj, anim_y_cb);
        
        // ? 第二步: 从对象中移除样式（在删除对象前）
        // 这会：
        // 1. 释放 obj->styles 数组
        // 2. 清理所有本地样式
        // 3. 处理转换和动画样式
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        
        // ? 第三步: 重置样式对象（可选，防止悬空引用）
        lv_style_reset(&Property->style);
        
        // ? 第四步: 删除对象
        // 此时 obj->styles 已为 NULL 或空
        lv_obj_del(obj);
        
        // ? 第五步: 释放 Property（包括其内部的样式）
        // 现在安全，因为没有地方再引用这个样式
        head->_free(Property);
        
        removeNode(head, node->data);
        ID_UPdata(head);
    }
}
```

**为什么有效**:
1. `lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY)` 会：
   - 遍历 `obj->styles` 数组
   - 对每个本地样式调用 `lv_style_reset()`
   - 调用 `lv_mem_free()` 释放每个本地样式对象
   - 调用 `lv_mem_realloc()` 缩小 `obj->styles` 数组
   - 清理转换和动画样式

2. 这样就不会有"指向已释放样式的 styles 数组"存在

3. 随后的 `lv_obj_del()` 和 `head->_free(Property)` 都是安全的

---

### 方案B: 独立管理样式生命周期 (更好的长期解决方案)

**修改 Table_Property 结构体**:

```c
// 当前 (问题代码):
typedef struct {
    int data;
    lv_obj_t* obj;
    lv_style_t style;  // ? 嵌入在结构体中
    // ...
} Table_Property;

// 改为:
typedef struct {
    int data;
    lv_obj_t* obj;
    lv_style_t* style;  // ? 单独指针
    uint8_t style_applied;  // ? 标记样式是否已应用
    // ...
} Table_Property;
```

**修改创建代码**:

```c
lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, 
                          void* updata_cd, uint16_t insert_index)
{
    // ...
    Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));
    
    // ? 单独分配样式对象
    table_proprity->style = lv_mem_alloc(sizeof(lv_style_t));
    if (table_proprity->style == NULL) {
        lv_mem_free(table_proprity);
        return NULL;
    }
    
    lv_style_init(table_proprity->style);
    lv_style_set_outline_width(table_proprity->style, 3);
    lv_style_set_outline_pad(table_proprity->style, 3);
    lv_style_set_outline_color(table_proprity->style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_font(table_proprity->style, &lv_font_montserrat_14);
    
    lv_obj_add_style(table, table_proprity->style, 0);  // ← 指针而不是地址
    
    table_proprity->style_applied = 1;
    table->user_data = table_proprity;
    
    return table;
}
```

**修改删除代码**:

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    _head = (List2*)head;
    Node* node = FindNode(head, index);
    if (node == NULL)
        return;
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        // ? 停止动画
        lv_anim_del(obj, anim_y_cb);
        
        // ? 移除样式
        if (Property->style_applied) {
            lv_obj_remove_style(obj, Property->style, 0);
            Property->style_applied = 0;
        }
        
        // ? 删除对象
        lv_obj_del(obj);
        
        // ? 释放样式对象
        if (Property->style) {
            lv_style_reset(Property->style);
            lv_mem_free(Property->style);
            Property->style = NULL;
        }
        
        // ? 最后释放 Property
        head->_free(Property);
        
        removeNode(head, node->data);
        ID_UPdata(head);
    }
}
```

**优势**:
- 样式生命周期明确
- 样式可以独立管理
- 不依赖 Property 的生命周期
- 更灵活，样式可以被多个对象共享

---

## ? 推荐实施步骤

### 步骤1: 立即应用方案A（最小改动）

编辑 UI_ListBox.c，在 `UI_Table_Clean` 函数中调整顺序：

```diff
  void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
  {
      _head = (List2*)head;
      Node* node = FindNode(head, index);
      if (node == NULL)
          return;
      if (node->instant)
      {
          Table_Property* Property = node->instant;
          lv_obj_t* obj = Property->obj;
          
+         // 停止动画
          lv_anim_del(obj, anim_y_cb);
+         
+         // 移除样式（必须在删除对象前）
+         lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
+         
          lv_style_reset(&Property->style);
-         lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
          lv_obj_del(obj);
          head->_free(Property);
```

**预期效果**: 立即修复 TLSF double-free 断言

### 步骤2: 验证修复

```c
// 添加日志验证
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    _head = (List2*)head;
    Node* node = FindNode(head, index);
    if (node == NULL)
        return;
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        LV_LOG_USER("[UI_Table_Clean] Cleaning table at %p, property at %p\n", obj, Property);
        
        lv_anim_del(obj, anim_y_cb);
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        lv_style_reset(&Property->style);
        lv_obj_del(obj);
        
        LV_LOG_USER("[UI_Table_Clean] Before free, obj=%p\n", obj);
        head->_free(Property);
        LV_LOG_USER("[UI_Table_Clean] After free\n");
        
        removeNode(head, node->data);
        ID_UPdata(head);
    }
}
```

### 步骤3: 后续改进（方案B）

在下个版本中逐步迁移到方案B，获得更好的长期可维护性。

---

## ? 修复检查清单

在应用修复后验证：

- [ ] `lv_anim_del()` 在 `lv_obj_remove_style()` 前调用
- [ ] `lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY)` 在 `lv_obj_del()` 前调用
- [ ] `lv_style_reset()` 在删除对象前或后都可以（现在位置不重要）
- [ ] 编译后无新警告
- [ ] 测试快速创建-删除表项（未出现断言）
- [ ] 测试容器删除时的清理循环（未出现卡死或断言）
- [ ] 检查日志中是否有重复的 Property 地址

---

## ? 预期改进

| 问题 | 修复前 | 修复后 |
|------|--------|--------|
| TLSF double-free | ? 断言失败 | ? 正常释放 |
| 样式泄漏 | ? 可能泄漏 | ? 完全清理 |
| 清理顺序 | ? 混乱 | ? 明确 |
| 代码可维护性 | ?? 困难 | ? 清晰 |

---

## ? 关键代码行数

需要修改的文件和行：

**文件**: `d:\workfile\项目3 vor\software\VOR_V3.0_AT32\INTERFACE\UI\UI_Component\UI_ListBox.c`

**函数**: `UI_Table_Clean()` (第 63-92 行)

**修改点**:
- L78: 添加 `lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);` 
- L79: 保留 `lv_style_reset(&Property->style);`
- L81: 保留 `lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);`（会变成重复，可删除）
- 或简化为一次调用

**简化修复（最小改动）**:

```c
// 删除重复的 lv_obj_remove_style 调用（原 L81）
// 在 L78（lv_anim_del 后）添加一次：
lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
```

这样就完成了修复！

