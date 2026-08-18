/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:16
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-11 11:12:32
 * @FilePath: \VOR_V3.0_AT32\INTERFACE\UI\UI_Component\UI_KeyBorad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UI_KeyBorad.h"
#include "../UI_Include.h"

typedef struct
{
	const char* keyChar;
	lv_btnmatrix_ctrl_t* matrix;
}UI_KeyboradItem;

static lv_obj_t* kb;
static int kbTypedSet = 0;

static const char* kb_map_num[] = {
	"1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
	"4", "5", "6", LV_SYMBOL_CLOSE, "\n",
	"7", "8", "9", ",", "\n",
	"0", "", "", "",
	NULL
};

// 按键映射控制（LV_BTNMATRIX_CTRL_* 可设置不同的按钮行为）
static const lv_btnmatrix_ctrl_t kb_ctrl_num_map[] = {
	1, 1, 1, LV_BTNMATRIX_CTRL_POPOVER,  // 第一行
	1, 1, 1, LV_BTNMATRIX_CTRL_POPOVER,  // 第二行
	1, 1, 1, LV_BTNMATRIX_CTRL_POPOVER,  // 第三行
	1, 0, 0, 0                           // 第四行
};

static const char* kb_map_num_char[] = {
	"1", "2", "3","A", LV_SYMBOL_BACKSPACE, "\n",
	"4", "5", "6","B", LV_SYMBOL_CLOSE, "\n",
	"7", "8", "9","C", ",", "\n",
	"0", "D" , "E","F","abc",
	NULL
};

// 按键映射控制（LV_BTNMATRIX_CTRL_* 可设置不同的按钮行为）
static const lv_btnmatrix_ctrl_t kb_map_num_char_map[] = {
	1, 1, 1,1, LV_BTNMATRIX_CTRL_POPOVER,  // 第一行
	1, 1, 1,1, LV_BTNMATRIX_CTRL_POPOVER,  // 第二行
	1, 1, 1,1, LV_BTNMATRIX_CTRL_POPOVER,  // 第三行
	1, 1, 1,1, 1                           // 第四行
};

//UI_KeyboradItem keyBoradList[] = {
//	{.keyChar = kb_map_num,.matrix = kb_ctrl_num_map},
//	{.keyChar = kb_map_num_char,.matrix = kb_map_num_char_map}
//};
void keyboradEvent(lv_event_t* e)
{
	lv_event_code_t code = e->code;
	if (code == LV_EVENT_CANCEL)
	{
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

	}
	//if(code== lv_evnet)
}



void del_custom_keyboard(void)
{
	if (kb)
	{
		lv_obj_del(kb);
		kb = 0;
	}
}

lv_obj_t* get_custom_keyboard(void)
{
	return kb;
}

// 创建自定义键盘
lv_obj_t* create_custom_keyboard(lv_obj_t* parent, int typed)
{

	if (kb == 0)
	{
		kb = lv_keyboard_create(parent);
		if (typed >= 2)
			typed = 0;
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER,
			kb_map_num_char, kb_map_num_char_map);
		kbTypedSet = typed;
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_event_cb(kb, keyboradEvent, LV_EVENT_ALL, 0);
	}
	if (typed != kbTypedSet)
	{
		if (typed >= 2)
			typed = 0;
		lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER,
			kb_map_num_char, kb_map_num_char_map);
		kbTypedSet = typed;
	}

	return kb;
}
