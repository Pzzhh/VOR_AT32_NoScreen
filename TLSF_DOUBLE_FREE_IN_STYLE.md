# TLSF Double-Free 发生在 lv_obj_set_local_style_prop 中的原因

## ? 问题核心

TLSF "block already marked as free" 断言失败发生在 **`lv_obj_set_local_style_prop`** → **`get_local_style`** → **`lv_mem_realloc`** 的调用链中。

这是一个**隐蔽的内存泄漏和重复释放问题**。

---

## ? 调用链分析

```
UI_ListBox.c 第310行:
  lv_obj_add_style(table, &table_proprity->style, 0);
  
  ↓
  
LVGL lv_obj.c:
  (某个地方设置样式)
  
  ↓
  
lv_obj_set_local_style_prop() 第270行
  {
    lv_style_t * style = get_local_style(obj, selector);  ← 问题发生在这里
    lv_style_set_prop(style, prop, value);
    lv_obj_refresh_style(obj, selector, prop);
  }
  
  ↓
  
get_local_style() 第505行
  {
    // 如果 style 不存在，需要分配新的
    obj->style_cnt++;
    obj->styles = lv_mem_realloc(obj->styles, 
                  obj->style_cnt * sizeof(_lv_obj_style_t));
                  
    // ← lv_mem_realloc 可能触发 TLSF double-free!
    
    obj->styles[i].style = lv_mem_alloc(sizeof(lv_style_t));
    lv_style_init(obj->styles[i].style);
    // ...
    return obj->styles[i].style;
  }
```

---

## ? 为什么会发生 Double-Free?

### 场景: 在 UI_Table_Clean 中的流程

```c
// UI_ListBox.c 第310行
lv_obj_add_style(table, &table_proprity->style, 0);
// ← 添加样式到对象

// ...后来...

// UI_ListBox.c 第80行
lv_obj_del(obj);  // ← 删除 LVGL 对象
// 此时 obj->styles 被释放

// 但是如果后续代码尝试:
lv_obj_set_local_style_prop(obj, ...)  // ← obj 已被删除！

结果:
  obj 的内存已被 TLSF 释放
  obj->styles 指向已释放的内存
  
  get_local_style() 尝试:
    obj->style_cnt++  ← 访问已释放内存
    lv_mem_realloc(obj->styles, ...)  ← 重新分配已释放的内存块
    
    这会触发 TLSF 的 double-free 检测!
```

### 更可能的场景: 对象未完全删除，样式操作持续

```
时间线:

T1: 创建表项，添加样式
    table = lv_table_create(parent)
    lv_obj_add_style(table, &table_proprity->style, 0)
    
    heap 分配:
    ┌──────────────────────┐
    │ LVGL 对象 table      │
    │ - styles: [ptr1]     │  ← 指向 styles 数组
    │ - style_cnt: 1       │
    └──────────────────────┘

T2: 清理表项
    lv_obj_del(table);  ← 删除对象
    
    heap 释放:
    ┌──────────────────────┐
    │ [已释放]             │  ← table 对象已删除
    │ [已释放]             │  ← styles 数组已删除
    └──────────────────────┘

T3: 但样式操作继续！(例如动画回调或事件处理)
    lv_obj_set_local_style_prop(table, prop, value, selector)
    
    get_local_style() 执行:
      obj->style_cnt++ ← 访问已释放对象 ?
      lv_mem_realloc(obj->styles, ...) 
                    ↑
                    指向已释放的 styles 数组
                    
    TLSF 检测:
      要重新分配的块已被标记为 free
      ↓
      tlsf_assert(!block_is_free(block)) 失败!
```

---

## ? 具体代码中的问题点

### 问题点1: 样式在对象删除前未清理

```c
// UI_ListBox.c 第310行
lv_obj_add_style(table, &table_proprity->style, 0);
// ↑ 添加样式
// 创建了内部的 styles 数组:
// table->styles = [ptr_to_style_object]
// table->style_cnt = 1

// ...

// UI_ListBox.c 第80行
lv_obj_del(obj);
// ↑ 删除对象
// 这会释放 table->styles 数组内存
// 但如果有代码继续访问 table 或其样式...

// ? 问题: 样式指针可能仍被引用
// 例如: table_proprity->style 仍被使用
```

### 问题点2: 样式数据结构的生命周期不匹配

```c
// UI_ListBox.c 第310行
lv_style_t style;
lv_style_init(&style);
lv_style_set_outline_width(&style, 3);
// ← style 是栈上的局部变量

lv_obj_add_style(table, &style, 0);
// ← 在 table->styles 数组中保存了 &style 的指针

// ... 函数返回，style 作用域结束...

// 但 table->styles[i].style 仍指向栈上的已无效的 style!

// 后续:
lv_obj_set_local_style_prop(table, ...)
  └─ get_local_style(table, ...)
    └─ obj->styles[i].style = ...  ← 指向无效的栈内存
    └─ lv_mem_realloc(obj->styles, ...)  ← 可能涉及这个无效指针
```

### 问题点3: lv_mem_realloc 的隐蔽问题

```c
// get_local_style() 第517行
obj->styles = lv_mem_realloc(obj->styles, 
              obj->style_cnt * sizeof(_lv_obj_style_t));
              
// lv_mem_realloc 的行为:
// 1. 如果能原地扩展，直接扩展
// 2. 如果不能原地扩展:
//    ├─ 分配新内存块
//    ├─ 复制旧数据
//    └─ 释放旧内存块  ← ? 可能触发 double-free!

// 如果旧的 obj->styles 已被释放过:
// realloc 会尝试释放已释放的内存
//   ↓
//   TLSF 检测到 block_is_free() = 1
//   ↓
//   tlsf_assert 失败!
```

---

## ? 在你的项目中最可能的具体场景

根据 UI_ListBox.c 的代码：

### 场景A: 样式栈变量问题 ?? 最可能

```c
// UI_ListBox.c 第315-326行
void UI_Table_Create(...) {
    // ...
    lv_obj_t* table = lv_table_create(parent);
    
    // ← table_proprity 指向堆分配的 Table_Property 结构
    Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));
    
    // ← style 是函数内部的栈变量
    lv_style_init(&table_proprity->style);
    lv_style_set_outline_width(&table_proprity->style, 3);
    lv_style_set_outline_pad(&table_proprity->style, 3);
    lv_style_set_outline_color(&table_proprity->style, lv_palette_main(LV_PALETTE_BLUE));
    
    // ← 添加 Table_Property 内的样式对象到 LVGL 表格
    lv_obj_add_style(table, &table_proprity->style, 0);
    //                           ^^^^^^^^^^^^^^^^^
    //                    指向 Table_Property 中的 style 字段
    
    table->user_data = table_proprity;
}

// 后来，删除时：
void UI_Table_Clean(...) {
    Table_Property* Property = node->instant;
    lv_obj_t* obj = Property->obj;
    
    lv_obj_del(obj);  ← 删除对象
    // ↓
    // 但 obj 内部的 obj->styles 数组仍可能引用 Property->style!
    
    head->_free(Property);  ← 释放包含 style 的结构体
    // ↓
    // 问题: Property->style 所在的内存被释放
    // 但如果某个地方还在引用这个样式...
    
    // 例如，如果 lv_obj_del 不完全，或有延迟清理:
    lv_obj_set_local_style_prop(obj, ...)  ← 访问已释放对象
      └─ get_local_style(obj, ...)
        └─ obj->styles[i].style = ...（指向已释放的 Property->style）
        └─ lv_mem_realloc(obj->styles, ...)  ← double-free!
```

**为什么这是 double-free:**

```
内存状态:

初始:
  ┌──────────────────────────────────┐
  │ Property (堆分配)                │
  │  ├─ style { ... }                │  ← style 在 Property 内
  │  └─ ...                          │
  └──────────────────────────────────┘

删除对象后:
  obj 的 styles 数组中仍有指向:
    obj->styles[0].style = &Property->style
    
随后 Property 被释放:
  ┌──────────────────────────────────┐
  │ [已释放]                          │
  │  ├─ [已释放]                     │  ← style 被释放
  │  └─ [已释放]                     │
  └──────────────────────────────────┘
  
但 obj->styles[0].style 仍指向这个地址!

再次访问样式:
  lv_obj_set_local_style_prop(obj, ...)
    └─ get_local_style(obj, ...)
      └─ 访问 obj->styles[0]（仍有效）
      └─ obj->styles[0].style 指向已释放的 Property->style
      └─ lv_mem_realloc(obj->styles, new_size)
          └─ TLSF 尝试重新分配已释放的内存块
          └─ block_is_free() = 1（已被释放）
          └─ tlsf_assert 失败!
```

### 场景B: 对象未正确删除 ?? 可能

```
lv_obj_del(obj) 可能不会完全删除对象:
  ├─ 某些系统中，删除是延迟的
  ├─ 某些属性可能仍被访问
  └─ styles 数组可能不被立即释放

结果: 对象的样式指针悬空，指向已释放的内存
```

### 场景C: 样式数组重新分配 ?? 高危

```c
get_local_style() 中:
  obj->style_cnt++;
  obj->styles = lv_mem_realloc(obj->styles, ...);
  // ↑
  // 如果样式数组之前被错误释放，这会导致 double-free
  
原因:
  1. obj->styles 指向已释放的内存块（地址 0x20012345）
  2. realloc 尝试释放这个块
  3. TLSF 检查：block_is_free(0x20012345) = true（已释放）
  4. tlsf_assert 失败
```

---

## ? 修复方案

### 修复1: 确保样式生命周期

```c
// ? 错误方式（样式在堆上分配）
typedef struct {
    lv_style_t style;  // ← 嵌入式样式
    // ...
} Table_Property;

// 后果: 释放 Property 时，style 也被释放
// 但 obj->styles 数组可能仍引用它

// ? 正确方式（样式独立管理）
typedef struct {
    lv_style_t* style;  // ← 样式指针
    // ...
} Table_Property;

// 在创建时：
Table_Property* prop = lv_mem_alloc(sizeof(Table_Property));
prop->style = lv_mem_alloc(sizeof(lv_style_t));
lv_style_init(prop->style);

// 在删除时：
// 先删除样式所有引用
lv_obj_remove_style(table, prop->style, 0);  ← 从对象中移除样式
// 然后释放样式
lv_mem_free(prop->style);
// 最后释放属性
lv_mem_free(prop);
```

### 修复2: 对象删除前清理样式

```c
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    if (node->instant) {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        // ? 第一步: 移除所有样式
        lv_obj_remove_style(obj, NULL, LV_STYLE_PROP_ANY);
        // ↑ 移除所有样式（包括动画和转换样式）
        
        // ? 第二步: 停止动画
        lv_anim_del(obj, anim_y_cb);
        
        // ? 第三步: 删除对象
        lv_obj_del(obj);
        
        // ? 第四步: 清理样式数据
        lv_style_reset(&Property->style);
        
        // ? 第五步: 释放 Property
        head->_free(Property);
    }
}
```

### 修复3: 防御性编程

```c
// 在 UI_Table_Create 中改进
lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, 
                          void* updata_cd, uint16_t insert_index)
{
    // ...
    Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));
    
    lv_style_init(&table_proprity->style);
    lv_style_set_outline_width(&table_proprity->style, 3);
    // ...
    
    // ? 关键: 添加样式后立即检查
    lv_obj_add_style(table, &table_proprity->style, 0);
    
    // ? 保存对象引用用于后续清理
    table_proprity->obj = table;
    
    // ? 标记样式已添加
    table_proprity->style_added = 1;
    
    return table;
}

// 删除时检查
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    if (node->instant) {
        Table_Property* Property = node->instant;
        
        // ? 检查样式是否已添加
        if (Property->style_added) {
            lv_obj_remove_style(Property->obj, &Property->style, 0);
            Property->style_added = 0;
        }
        
        // ...
        lv_obj_del(Property->obj);
        head->_free(Property);
    }
}
```

### 修复4: lv_obj_remove_style 的正确使用

```c
// ? 完整的样式清理
void remove_all_local_styles(lv_obj_t* obj) {
    // 移除特定样式
    lv_obj_remove_style(obj, NULL, LV_PART_ANY);
    
    // 确保所有转换和动画样式也被移除
    // (这由 lv_obj_remove_style 处理)
}

// 在 UI_Table_Clean 中使用
lv_obj_remove_style(obj, NULL, LV_PART_ANY);
// ↑ 移除所有本地样式（part = ANY 表示所有部分）
```

---

## ? 问题根源总结

| 问题 | 代码位置 | 原因 | 结果 |
|------|---------|------|------|
| 样式生命周期 | UI_ListBox.c L315 | style 嵌入在 Property | 释放后悬空指针 |
| 样式未清理 | UI_ListBox.c L80 | lv_obj_del 前无样式移除 | obj->styles 指向释放内存 |
| realloc double-free | lv_obj_style.c L517 | 对已释放的数组 realloc | TLSF 检测 double-free |
| 延迟清理 | 事件回调 | obj 删除后仍访问样式 | 访问释放内存 |

---

## ? 最高优先级修复

```c
// 在 lv_obj_del(obj) 前添加这行
lv_obj_remove_style(obj, NULL, LV_PART_ANY);

// 这会：
// 1. 移除所有本地样式
// 2. 释放 obj->styles 数组
// 3. 清理所有样式相关的转换和动画
// 4. 防止后续的 double-free
```

这个单一修改应该能解决问题，因为它防止了 LVGL 内部对已释放样式数据的访问。

