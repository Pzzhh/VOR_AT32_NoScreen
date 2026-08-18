#include "UI_ListBox.h"
#include "../other/list.h"

/* ========== 调试用：记录创建的对象地址，用于 Keil 设置内存断点 ========== */
/* 在 Keil Watch 窗口观察这些变量，获取要监控的地址 */
volatile lv_obj_t* dbg_last_created_table = NULL;    // 最后创建的 table 对象
volatile uint32_t  dbg_table_parent_addr = 0;         // table->parent 的地址 (用于设置写断点)
volatile int       dbg_table_create_count = 0;        // 创建计数器
/* ========================================================================= */

// 声明一个新的静态事件处理函数
static void listbox_container_event_handler(lv_event_t* e);


static void anim_y_cb(void* var, int32_t v) {
	lv_obj_align((lv_obj_t*)var, LV_ALIGN_DEFAULT, 0, v);
	//lv_obj_set_y((lv_obj_t*)var, v);
}

// 自定义动画路径函数，例如 y = t^2
static int32_t custom_anim_path(const lv_anim_t* a) {
	static float movement_factor = 0.14;
	Table_Property* table = a->user_data;
	if (table->isremove)
	{
		return 0;
	}
	if (table->updata_cd != 0)
	{
		table->updata_cd(table);
	}
	if (table->set_pos > 1000 || table->set_clearence > 1000)
	{
		return 0;
	}
	table->set_pos = table->set_clearence * (*table->ID);
	table->pos_y += (table->set_pos - table->pos_y) * movement_factor;
	//lv_group_focus_obj(table->obj);
	return  (int32_t)table->pos_y;
}

int UI_Table_Get_Fouces(Table_Contain_Property* table_Contain_Property)
{
	/*Node* box_node = FindNode(head, 0);
	lv_obj_t* table_contain = box_node->instant;
	Table_Contain_Property* table_Contain_Property = table_contain->user_data;*/
	return table_Contain_Property->hightLight_index;
}

void UI_Table_Set_Fouces(Table_Contain_Property* table_Contain_Property, int index)
{
	table_Contain_Property->hightLight_index = index;
	lv_obj_invalidate(table_Contain_Property->obj);
}

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
List2* _head;

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
		
		/* 
		 * 修复：在删除对象或释放其属性之前，必须先显式地删除动画。
		 * lv_anim_del 通过被动画化的变量(var)和执行回调(exec_cb)来查找并删除动画。
		 * 在这里，'var' 就是 table 对象 'obj'。
		 */
		lv_anim_del(obj, anim_y_cb);
		
		/* 
		 * 修复内存破坏问题：必须先从对象移除样式引用，再释放 style 内存。
		 * 之前的顺序是先 lv_style_reset 再 lv_obj_remove_style，这会导致
		 * lv_obj_remove_style 或 lv_obj_del 访问已释放的 style->v_p.values_and_props
		 */
		lv_obj_remove_style(obj, NULL, LV_PART_ANY | LV_STATE_ANY);  // 1. 先移除样式引用
		lv_style_reset(&Property->style);                            // 2. 再释放 style 内部内存
		
		lv_obj_del(obj);
		head->_free(Property);
		removeNode(head, node->data);
		ID_UPdata(head);

		LV_LOG_USER("\r\nremove Index addr %x", Property->obj);
	}
}

uint8_t UI_Table_Find_Obj(List* head, lv_obj_t* obj, int* index)
{
	int counter = 0;
	Node* node = 0;
	while (1)
	{
		node = FindNode(head, counter++);
		if (node->next == NULL)
			return 0;
		if (node->instant != 0)
		{
			Table_Property* ins = node->instant;
			if (ins->obj == obj)
			{
				*index = counter;
				return 1;
			}
		}
	}
}

int UI_Table_Find_Size(Table_Contain_Property* table_Contain_Property)
{
	List* head = table_Contain_Property->list;
	return head->size;
}

Table_Property* UI_Table_Find_Obj_User_Data(Table_Contain_Property* table_Contain_Property, int index)
{
	List* head = table_Contain_Property->list;
	Node* node = 0;
	Table_Property* ins = 0;
	node = FindNode(head, index);
	if (node == NULL)
		return ins;
	if (node->instant != 0)
	{
		ins = node->instant;
	}
	return ins;
}

Table_Property* UI_Table_Get_Property(lv_obj_t* obj)
{
	return (Table_Property*)obj->user_data;
}

void UI_Table_Set_Property(lv_obj_t* obj, void* src)
{
	obj->user_data = src;
	return;
}


static void Tabels_Touch_handler(lv_event_t* e);
///// event handler
void UI_Table_Clean_event(lv_event_t* e)
{
	lv_event_code_t code = e->code;
	List* head = e->user_data;
	// List2* _head = (List2*)head;
	// lv_obj_t* table = e->current_target;
	int index = 0;
	if (code == LV_EVENT_DELETE)
	{
		//if (UI_Table_Find_Obj(head, table, &index) != 1)
		//    return;
		Node* node = FindNode(head, index);
		//if (node->instant)
		//{
		//    Table_Property* Property = node->instant;
		//    //Property->obj->user_data = 0;
		//    head->_free(Property);
		//    //removeNode(head, index);
		//}

	}
}

static void Tabels_Touch_handler(lv_event_t* e)
{
	static Table_Property* table_Property2;
	// Node* pt = e->user_data;
	// List* head = e->user_data;
	lv_event_code_t code = e->code;
	lv_obj_t* table = e->current_target;
	lv_obj_t* table_container = lv_obj_get_parent(table);
	Table_Contain_Property* table_Contain_Property = table_container->user_data;
	Table_Property* table_Property = table->user_data;
	if (table_Property == table_Property2 || table_Property == 0)
	{
		LV_LOG_USER("code %d", code);
		return;
	}
	UI_Table_Clean_event(e);
	if (code == LV_EVENT_CLICKED)
	{
		table_Contain_Property->hightLight_index = *table_Property->ID;
		//lv_styo
		lv_style_t* style = &table_Property->style;
		//lv_style_set_outline_width(style, 3);
		//lv_style_set_outline_color(style, lv_palette_main(LV_PALETTE_BLUE));
		lv_style_set_outline_opa(style, 0xff);
		/*    lv_style_set_outline_pad(style, 3);*/
		lv_obj_invalidate(table_container);
	}
	else  if (code != LV_EVENT_DELETE && table_Contain_Property->hightLight_index != *table_Property->ID)
	{
		lv_style_t* style = &table_Property->style;
		//lv_style_set_outline_width(style, 3);
		//lv_color_t color = { .ch.blue = 0xff,.ch.green = 0xff,.ch.red = 0xff };
		////lv_style_set_outline_color(style, color);
		lv_style_set_outline_opa(style, 5);
		//lv_style_set_outline_pad(style, 3);
	}

}
//end// event handler

void UI_Table_Move(Table_Contain_Property* table_Contain_Property, int fromIndex, int toIndex)
{
	List* list = table_Contain_Property->list;
	move(list, fromIndex, toIndex);
	ID_UPdata(list);
}

// 定义新的事件处理函数
static void listbox_container_event_handler(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* obj = lv_event_get_target(e);

	if (code == LV_EVENT_DELETE) {
		LV_LOG_USER("ListBox container deleted, cleaning up custom data.");
		Table_Contain_Property* prop = (Table_Contain_Property*)lv_obj_get_user_data(obj);
		if (prop) {
			// 确保所有子项都已被清理
			while (prop->list->size > 0) {
				UI_Table_Clean(prop->list, 0, obj);
			}
			// 释放 List 结构体本身
			lv_mem_free(prop->list);
			// 释放 Table_Contain_Property 结构体
			lv_mem_free(prop);
		}
	}
}

Table_Contain_Property* UI_ListBox_Create(lv_obj_t* parent)
{
	List* list = lv_mem_alloc(sizeof(List));
	lv_memset_00(list, sizeof(List));
	List_init(list);
	list->_free = lv_mem_free;
	list->_malloc = lv_mem_alloc;

	lv_obj_t* table_box = lv_obj_create(parent);
	lv_obj_set_size(table_box, 450, 300);
	lv_obj_align(table_box, LV_ALIGN_CENTER, 150, 0);
	Table_Contain_Property* table_Contain_Property = lv_mem_alloc(sizeof(Table_Contain_Property));
	//table_Contain_Property->hightLight_index = 1;       //set the first item 
	table_box->user_data = table_Contain_Property;
	table_Contain_Property->list = list;
	table_Contain_Property->obj = table_box;
	table_Contain_Property->hightLight_index = 0;
	
	// 添加事件回调以处理内存释放
	lv_obj_add_event_cb(table_box, listbox_container_event_handler, LV_EVENT_DELETE, NULL);

	return table_Contain_Property;
}



//if insert_index == 0xffff auto insert append
lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, void* updata_cd, uint16_t insert_index)
{
	lv_obj_t* parent = table_Contain_Property->obj;
	List* head = table_Contain_Property->list;
	lv_obj_t* table = lv_table_create(parent);


	Table_Property* table_proprity = lv_mem_alloc(sizeof(Table_Property));
	Node* newnode = 0;
	if (insert_index == 0xffff)
	{
		newnode = add(head, table_proprity);
	}
	else
	{
		newnode = insert(head, insert_index, table_proprity);
		if (newnode == 0)
			newnode = add(head, table_proprity);
	}
	lv_memset_00(table_proprity, sizeof(Table_Property));

	lv_obj_set_height(table, 50);
	lv_table_set_col_width(table, 0, 90);
	lv_table_set_col_width(table, 1, 90);
	for (int k = 2; k < 4; k++)
		lv_table_set_col_width(table, k, 110);
	lv_obj_clear_flag(table, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(table, 10, NULL);
	lv_style_init(&table_proprity->style);
	lv_style_set_outline_width(&table_proprity->style, 3);
	lv_style_set_outline_pad(&table_proprity->style, 3);
	lv_style_set_outline_color(&table_proprity->style, lv_palette_main(LV_PALETTE_BLUE));
	lv_style_set_text_font(&table_proprity->style, &lv_font_montserrat_14);
	lv_obj_add_style(table, &table_proprity->style, 0);
	table->user_data = table_proprity;
	

	lv_anim_t a, * anim;
	lv_anim_init(&a);
	a.user_data = table_proprity;
	lv_anim_set_var(&a, table);
	lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_exec_cb(&a, anim_y_cb); // 设置执行回调函数
	lv_anim_set_path_cb(&a, custom_anim_path); // 设置自定义路径回调函数
	anim = lv_anim_start(&a); // 开始动画
	lv_obj_add_event_cb(table, Tabels_Touch_handler, LV_EVENT_ALL, head);


	table_proprity->ID = &newnode->data;
	table_proprity->obj = table;
	table_proprity->set_clearence = 75;
	table_proprity->updata_cd = updata_cd;
	table_proprity->isremove = 0;
	
	/* ========== 调试：记录对象地址用于 Keil 断点 ========== */
	dbg_last_created_table = table;
	dbg_table_parent_addr = (uint32_t)&(table->parent);  // parent 成员的地址
	dbg_table_create_count++;
	/* 
	 * 在 Keil 中设置 Access Breakpoint:
	 * 1. 进入调试模式后，在 Watch 窗口查看 dbg_table_parent_addr 的值
	 * 2. Debug -> Breakpoints -> New (Access)
	 * 3. Address: 填入 dbg_table_parent_addr 的值
	 * 4. Size: 4, Type: Write
	 * 这样当有任何代码修改 table->parent 时就会中断
	 */
	/* ======================================================= */
	
	/* if (*table_proprity->ID > 2)
	 {
		 table_proprity->pos_y = table_proprity->set_clearence * (*table_proprity->ID - 2);
		 lv_obj_align(table, LV_ALIGN_DEFAULT, 0, table_proprity->pos_y);
	 }*/
	table_proprity->anim = anim;
	ID_UPdata(head);
	return table;
}

