# UI_Table_Clean TLSF Double-Free 根因分析与修复

## ? 根本原因确认

经过分析，问题的根本原因是 `UI_Table_Clean` 中的内存清理顺序问题，导致 LVGL 堆被破坏。

### 内存布局分析

```
UI_Table_Create 分配的内存:
┌─────────────────────────────────────────┐
│ Table_Property (通过 lv_mem_alloc)      │
│ ┌─────────────────────────────────────┐ │
│ │ ID, obj, set_clearence, ...         │ │
│ ├─────────────────────────────────────┤ │
│ │ lv_style_t style  <── 嵌入的样式   │ │  ← lv_obj_add_style() 使用此地址
│ │   (outline_width, color, font...)   │ │
│ ├─────────────────────────────────────┤ │
│ │ anim, isremove, ...                 │ │
│ └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
         ↓
   table->styles 数组中存储 &Property->style 的指针
```

### 问题序列

```
时间线:

T1. UI_Table_Create():
    - lv_mem_alloc(sizeof(Table_Property))  → Property
    - lv_style_init(&Property->style)
    - lv_obj_add_style(table, &Property->style, 0)
      → table->styles[n].style = &Property->style

T2. UI_Table_Clean():
    - lv_style_reset(&Property->style)      ← 清空样式内容 (OK)
    - lv_obj_remove_style(obj, NULL, ...)   ← 从 obj->styles 移除 (OK)
    - lv_obj_del(obj)                       ← 删除 table 对象 (OK)
    - head->_free(Property)                 ← lv_mem_free(Property)
      
      此时 Property (包括 &Property->style) 内存块被释放
      TLSF 将此块标记为 FREE，并可能与相邻块合并

T3. 反复执行 T1, T2...
    → LVGL 堆出现碎片化
    → 某些释放的块可能被重新分配给其他用途

T4. UI_Page1_Btn_Refulsh():
    - lv_obj_set_style_bg_color(start_btn, ...)
      → lv_obj_set_local_style_prop()
        → get_local_style()
          → lv_mem_realloc(start_btn->styles, ...)
            
            ? 如果 realloc 操作涉及到之前被破坏的堆区域
               TLSF 检测到 block_is_free(block) == true
               但该块不应该是 free 状态 (元数据不一致)
               触发断言: "block already marked as free"
```

### 为什么 start_btn/stop_btn 会受影响?

这是典型的 **堆损坏传播** 问题：

1. Table 的 Property 释放时，如果清理顺序不对，可能破坏 TLSF 元数据
2. LVGL 使用单一堆管理所有内存（对象、样式数组、动画等）
3. start_btn 的 styles 数组与之前释放的 Property 可能在同一堆区域
4. 当 realloc 需要扩展或移动内存时，遇到损坏的元数据

---

## ? 修复方案

### 修复 1: 调整 UI_Table_Clean 的清理顺序 [关键]

**文件**: `INTERFACE/UI/UI_Component/UI_ListBox.c`

**当前代码** (第 63-95 行):
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
        lv_style_reset(&Property->style);               // ? 顺序问题
        lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
        lv_obj_del(obj);
        head->_free(Property);
        removeNode(head, node->data);
        ID_UPdata(head);

        LV_LOG_USER("\r\nremove Index addr %x", Property->obj);  // ? Use-After-Free
    }
}
```

**修复后代码**:
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
        
        // ? 步骤1: 保存日志需要的信息（在释放前）
        void* obj_addr_for_log = Property->obj;
        
        // ? 步骤2: 停止动画（防止回调访问已释放对象）
        lv_anim_del(obj, anim_y_cb);
        
        // ? 步骤3: 先移除样式引用（让 LVGL 正确清理内部引用）
        // 这一步必须在 lv_style_reset 之前，确保 obj->styles 数组正确更新
        lv_obj_remove_style(obj, &Property->style, LV_PART_ANY | LV_STATE_ANY);
        
        // ? 步骤4: 重置样式内容（清空样式数据）
        lv_style_reset(&Property->style);
        
        // ? 步骤5: 删除 LVGL 对象
        lv_obj_del(obj);
        
        // ? 步骤6: 清空节点指针（防止悬空引用）
        node->instant = NULL;
        
        // ? 步骤7: 从链表移除节点
        removeNode(head, node->data);
        ID_UPdata(head);
        
        // ? 步骤8: 最后释放 Property
        head->_free(Property);
        
        // ? 步骤9: 使用之前保存的地址记录日志
        LV_LOG_USER("\r\nremove Index addr %x", obj_addr_for_log);
    }
}
```

### 关键修改点

| 项目 | 原代码 | 修复后 | 原因 |
|------|--------|--------|------|
| 移除样式 | 在 style_reset 后 | 在 style_reset 前 | 让 LVGL 先清理内部引用 |
| 移除样式参数 | `NULL` (移除全部) | `&Property->style` (指定) | 更精确，避免误删 |
| 释放 Property | 在 removeNode 前 | 在 removeNode 后 | 确保链表操作完成 |
| 日志 | 在 free 后访问 | 保存副本后访问 | 修复 Use-After-Free |
| 清空 instant | 无 | 添加 `node->instant = NULL` | 防止悬空指针 |

---

## ? 代码变更 (可直接应用)

```diff
--- a/INTERFACE/UI/UI_Component/UI_ListBox.c
+++ b/INTERFACE/UI/UI_Component/UI_ListBox.c
@@ -70,21 +70,32 @@ void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
 	{
 		Table_Property* Property = node->instant;
 		lv_obj_t* obj = Property->obj;
-		//lv_obj_style
-		/* 
-		 * 修复：在删除对象或释放其属性之前，必须先显式地删除动画。
-		 * lv_anim_del 通过被动画化的变量(var)和执行回调(exec_cb)来查找并删除动画。
-		 * 在这里，'var' 就是 table 对象 'obj'。
-		 */
-			//lv_anim_wait
-		//lv_animimg_set_src
+		
+		// 保存日志需要的信息（在释放前）
+		void* obj_addr_for_log = Property->obj;
+		
+		// 步骤1: 停止动画
 		lv_anim_del(obj, anim_y_cb);
-		lv_style_reset(&Property->style);
-		/* 
-		 * 现在动画已经停止，以下清理步骤是安全的。
-		 * Property->isremove 和 Property->anim->repeat_cnt = 0 的操作也就不再需要了。
-		 */
-		lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);
+		
+		// 步骤2: 先移除样式引用（必须在 style_reset 之前）
+		lv_obj_remove_style(obj, &Property->style, LV_PART_ANY | LV_STATE_ANY);
+		
+		// 步骤3: 重置样式内容
+		lv_style_reset(&Property->style);
+		
+		// 步骤4: 删除 LVGL 对象
 		lv_obj_del(obj);
-		head->_free(Property);
+		
+		// 步骤5: 清空节点指针
+		node->instant = NULL;
+		
+		// 步骤6: 从链表移除
 		removeNode(head, node->data);
 		ID_UPdata(head);
+		
+		// 步骤7: 最后释放 Property
+		head->_free(Property);
 
-		LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
-		////head->_free(Property);
-		//removeNode(head, node->data);
+		LV_LOG_USER("\r\nremove Index addr %x", obj_addr_for_log);
 	}
 }
```

---

## ? 验证测试

修复后请执行以下测试:

```
测试场景:
1. 启动程序，进入 Page1
2. 添加 5 个任务项 (点击 ADD 按钮)
3. 删除所有任务项 (点击 DEL 按钮)
4. 重复步骤 2-3 共 20 次
5. 每次删除后，观察系统是否正常运行
6. 检查是否还有 TLSF 断言失败

预期结果:
- 不再出现 "block already marked as free" 断言
- UI 刷新正常，按钮样式更新正常
```

---

## ? 附加建议

### 1. 添加堆健康检查 (可选)

在关键操作前后添加堆检查，帮助定位问题:

```c
// 在 lv_conf.h 或调试代码中
#if LV_USE_ASSERT_MEM_INTEGRITY
    extern void lv_mem_test(void);
    #define LV_MEM_CHECK() lv_mem_test()
#else
    #define LV_MEM_CHECK()
#endif

// 在 UI_Table_Clean 开始和结束时调用
void UI_Table_Clean(List* head, int index, lv_obj_t* parent_)
{
    LV_MEM_CHECK();  // 检查堆完整性
    // ... 清理代码 ...
    LV_MEM_CHECK();  // 再次检查
}
```

### 2. 考虑使用动态分配的样式

如果嵌入式样式继续造成问题，可以改用动态分配:

```c
// 在 UI_Table_Create 中:
table_proprity->style_ptr = lv_mem_alloc(sizeof(lv_style_t));
lv_style_init(table_proprity->style_ptr);
lv_obj_add_style(table, table_proprity->style_ptr, 0);

// 在 UI_Table_Clean 中:
lv_obj_remove_style(obj, Property->style_ptr, ...);
lv_style_reset(Property->style_ptr);
lv_mem_free(Property->style_ptr);  // 单独释放
Property->style_ptr = NULL;
// 然后释放 Property
```

---

## ? 总结

| 问题 | 根因 | 修复 |
|------|------|------|
| TLSF double-free | 样式清理顺序不正确 | 先 remove_style，后 style_reset |
| 堆损坏传播 | 释放后访问 Property | 保存副本，最后释放 |
| 悬空指针 | node->instant 未清空 | 释放前置 NULL |

**核心修复**: 调整 `UI_Table_Clean` 中的清理顺序，确保 LVGL 内部引用在释放内存前被正确清理。
