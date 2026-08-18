# UI_Page1_Btn_Refulsh 中的 TLSF Double-Free 问题分析

## ? 问题定位

**函数**: `UI_Page1_Btn_Refulsh()` (page1.c 第 740-834 行)

**断言失败位置**: 
- `lv_obj_set_local_style_prop()` → `get_local_style()` → `lv_mem_realloc()`
- 错误: `tlsf_assert(!block_is_free(block) && "block already marked as free")`

**触发条件**: 多次执行 `UI_Table_Create()` 和 `UI_Table_Clean()` 后，在 `UI_Page1_Btn_Refulsh()` 中调用 `lv_obj_set_style_bg_color()` 时崩溃。

---

## ? 问题代码

```c
void UI_Page1_Btn_Refulsh()
{
    // ... 每次 flag 变化时 ...
    
    if (flag.IsRunning)
    {
        if (flag.isPause && flag.reqPause == 0)
        {
            uint32_t color = (count % 24 > 12) ? 0x7CFC00 : 0xF9F900;
            lv_obj_set_style_bg_color(start_btn, lv_color_hex(color), 0);  // ← 触发点
            lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEA0000), 0);
        }
        // ... 更多 lv_obj_set_style_bg_color 调用 ...
    }
    
    // ... 对 Task_Btn_Array 的多次样式设置 ...
    for (i = 0; i < 4; i++)
    {
        lv_obj_set_style_bg_color(Task_Btn_Array[i], lv_color_hex(0xADADAD), 0);  // ← 触发点
    }
}
```

---

## ? 根本原因分析

### 问题流程

```
1. 多次 UI_Table_Create() 和 UI_Table_Clean() 操作
   ↓
2. LVGL 堆内存发生碎片化
   ↓
3. UI_Table_Clean() 中的样式清理不完整
   ↓
4. 某些内存块被错误释放或标记
   ↓
5. UI_Page1_Btn_Refulsh() 被 timer 定期调用
   ↓
6. lv_obj_set_style_bg_color() 调用 lv_obj_set_local_style_prop()
   ↓
7. get_local_style() 尝试扩展 obj->styles 数组
   ↓
8. lv_mem_realloc() 操作已被错误释放的内存块
   ↓
9. TLSF 检测到 block_is_free() = 1
   ↓
10. 断言失败!
```

### 为什么 `lv_obj_set_style_bg_color` 会触发问题?

```c
// LVGL 内部实现
void lv_obj_set_style_bg_color(lv_obj_t* obj, lv_color_t value, lv_style_selector_t selector)
{
    lv_style_value_t v = { .color = value };
    lv_obj_set_local_style_prop(obj, LV_STYLE_BG_COLOR, v, selector);
    //                          ↑
    //                    这里触发问题
}

void lv_obj_set_local_style_prop(lv_obj_t* obj, ...)
{
    lv_style_t* style = get_local_style(obj, selector);
    //                  ↑
    //            这里可能触发 realloc
    lv_style_set_prop(style, prop, value);
    lv_obj_refresh_style(obj, selector, prop);
}

static lv_style_t* get_local_style(lv_obj_t* obj, lv_style_selector_t selector)
{
    // 如果找不到已有的本地样式，需要创建新的
    obj->style_cnt++;
    obj->styles = lv_mem_realloc(obj->styles, obj->style_cnt * sizeof(_lv_obj_style_t));
    //            ↑
    //      如果 obj->styles 指向已被释放的内存块
    //      lv_mem_realloc 会尝试释放它
    //      触发 TLSF double-free 断言
    
    // ...
}
```

### 问题的真正来源

问题不是 `UI_Page1_Btn_Refulsh` 本身，而是 **之前的 `UI_Table_Clean` 操作破坏了 LVGL 堆的一致性**。

```
时间线:

T1: UI_Table_Create() 创建表项
    ├─ 分配 Table_Property
    ├─ 分配 LVGL 对象
    └─ 分配 obj->styles 数组

T2: UI_Table_Clean() 删除表项
    ├─ lv_obj_del(obj)  ← 释放 obj 和它的 styles
    └─ head->_free(Property)  ← 释放 Property
    
    但是！在 UI_ListBox.c 中:
    ├─ 样式清理顺序可能不正确
    ├─ 某些 style 可能被重复释放
    └─ 或者 obj->styles 数组已释放但仍被引用

T3: UI_Page1_Btn_Refulsh() 被 timer 调用
    ├─ 对 start_btn 设置样式
    │   └─ lv_obj_set_style_bg_color(start_btn, ...)
    │       └─ get_local_style()
    │           └─ lv_mem_realloc()  ← 操作已损坏的堆
    │               └─ ? TLSF 断言失败!
    │
    └─ 堆已被 UI_Table_Clean 破坏，这里只是触发点
```

---

## ? 真正的问题位置

### 问题在 UI_ListBox.c 的 UI_Table_Clean 函数

```c
// UI_ListBox.c 第 63-92 行
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    // ...
    if (node->instant)
    {
        Table_Property* Property = node->instant;
        lv_obj_t* obj = Property->obj;
        
        lv_anim_del(obj, anim_y_cb);
        lv_style_reset(&Property->style);       // ← 重置样式内容
        
        // ? 问题: lv_obj_remove_style 在删除对象后不一定有效
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        
        lv_obj_del(obj);                        // ← 删除对象
        head->_free(Property);                  // ← 释放 Property (包含 style)
        
        // ? Use-After-Free: Property 已释放
        LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
    }
}
```

**问题**:
1. `lv_style_reset()` 重置了样式内容，但样式对象仍在 `obj->styles` 数组中
2. `lv_obj_del(obj)` 删除对象，但可能不会完全清理所有样式引用
3. `head->_free(Property)` 释放了包含 `Property->style` 的内存
4. 但 LVGL 内部的某些数据结构可能仍引用这个样式

---

## ? 修复方案

### 方案 A: 修复 UI_Table_Clean 的清理顺序 ? 最直接

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
        
        // ? 步骤1: 停止动画（防止动画回调访问已释放的对象）
        lv_anim_del(obj, anim_y_cb);
        
        // ? 步骤2: 移除所有样式（必须在删除对象前！）
        // 这会正确清理 obj->styles 数组
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        
        // ? 步骤3: 重置 Property 中的样式对象
        lv_style_reset(&Property->style);
        
        // ? 步骤4: 保存日志信息（在删除前）
        LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
        
        // ? 步骤5: 删除 LVGL 对象
        lv_obj_del(obj);
        
        // ? 步骤6: 清除节点中的指针（防止悬空引用）
        node->instant = NULL;
        
        // ? 步骤7: 释放 Property
        head->_free(Property);
        
        // ? 步骤8: 从列表中移除节点
        removeNode(head, node->data);
        ID_UPdata(head);
    }
}
```

### 方案 B: 在 page1.c 中添加防护检查 ? 补充方案

```c
void UI_Page1_Btn_Refulsh()
{
    // ... 现有代码 ...
    
    // ? 在设置样式前检查对象有效性
    if (start_btn != NULL && lv_obj_is_valid(start_btn))
    {
        lv_obj_set_style_bg_color(start_btn, lv_color_hex(color), 0);
    }
    
    if (stop_btn != NULL && lv_obj_is_valid(stop_btn))
    {
        lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEA0000), 0);
    }
    
    // ... 对其他对象也添加类似检查 ...
}
```

### 方案 C: 使用静态样式而不是本地样式 ? 根本解决

问题的根源是 `lv_obj_set_style_bg_color()` 会创建/扩展本地样式数组。如果使用预定义的静态样式，可以避免频繁的内存分配。

```c
// 在文件顶部定义静态样式
static lv_style_t style_btn_green;
static lv_style_t style_btn_red;
static lv_style_t style_btn_yellow;
static lv_style_t style_btn_gray;
static bool styles_initialized = false;

void init_button_styles(void)
{
    if (styles_initialized) return;
    
    lv_style_init(&style_btn_green);
    lv_style_set_bg_color(&style_btn_green, lv_color_hex(0x7CFC00));
    
    lv_style_init(&style_btn_red);
    lv_style_set_bg_color(&style_btn_red, lv_color_hex(0xEA0000));
    
    lv_style_init(&style_btn_yellow);
    lv_style_set_bg_color(&style_btn_yellow, lv_color_hex(0xF9F900));
    
    lv_style_init(&style_btn_gray);
    lv_style_set_bg_color(&style_btn_gray, lv_color_hex(0xADADAD));
    
    styles_initialized = true;
}

void UI_Page1_Btn_Refulsh()
{
    // ... 现有代码 ...
    
    if (flag.IsRunning)
    {
        if (flag.isPause && flag.reqPause == 0)
        {
            // ? 使用静态样式替换本地样式
            lv_obj_remove_style(start_btn, NULL, 0);
            if (count % 24 > 12) {
                lv_obj_add_style(start_btn, &style_btn_green, 0);
            } else {
                lv_obj_add_style(start_btn, &style_btn_yellow, 0);
            }
            
            lv_obj_remove_style(stop_btn, NULL, 0);
            lv_obj_add_style(stop_btn, &style_btn_red, 0);
        }
        // ... 类似替换其他位置 ...
    }
}
```

---

## ? 修复优先级

| 方案 | 位置 | 改动量 | 效果 | 推荐度 |
|------|------|--------|------|--------|
| **方案A** | UI_ListBox.c | 小 | 根本修复 | ????? |
| 方案B | page1.c | 小 | 防护检查 | ??? |
| 方案C | page1.c | 大 | 避免问题 | ???? |

**推荐**: 首先应用**方案A**修复 UI_ListBox.c，这是根本解决方案。

---

## ? 立即修复步骤

### 步骤1: 修改 UI_ListBox.c 中的 UI_Table_Clean 函数

将清理顺序调整为：
1. `lv_anim_del()` - 停止动画
2. `lv_obj_remove_style()` - 移除所有样式 (**提前到删除对象前**)
3. `lv_style_reset()` - 重置样式内容
4. `lv_obj_del()` - 删除对象
5. `head->_free()` - 释放数据结构

### 步骤2: 验证修复

```
测试场景:
1. 启动程序
2. 创建 5 个表项 (UI_Task_Btn_ADD_Click_Event)
3. 删除所有表项 (UI_Task_Btn_Del_Click_Event)
4. 重复步骤 2-3 多次 (10 次以上)
5. 观察是否还有 TLSF 断言失败
```

---

## ? 关键总结

**问题原因**: `UI_Table_Clean` 中的样式清理顺序不正确，导致 LVGL 堆被破坏。

**触发点**: `UI_Page1_Btn_Refulsh` 中的 `lv_obj_set_style_bg_color` 调用只是触发了已经被破坏的堆。

**根本修复**: 调整 `UI_Table_Clean` 中的清理顺序，确保 `lv_obj_remove_style()` 在 `lv_obj_del()` 之前调用。

