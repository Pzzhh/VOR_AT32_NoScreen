/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-08 11:52:54
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\UI_Component\UI_SpinBox.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UI_SpinBox.h"

typedef enum
{
	SpinBox_None,
	SpinBox_increase,
	SpinBox_decrease
} SpinBox_Enum;

void SpinBox_Btn_handler(lv_event_t* e)
{
	lv_obj_t* SpinBox = (lv_obj_t*)(e->user_data);
	int32_t* value = SpinBox->user_data;
	lv_event_code_t code = lv_event_get_code(e);
	int Call_ID = (int)e->current_target->user_data;
	// lv_obj_t *obj = lv_event_get_target(e);
	if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
	{
		if (Call_ID == SpinBox_increase)
		{
			lv_spinbox_increment(SpinBox);
		}
		else if (Call_ID == SpinBox_decrease)
		{
			lv_spinbox_decrement(SpinBox);
		}
		if (value != 0)
			*value = lv_spinbox_get_value(SpinBox);
		LV_LOG_USER(" %d", *value);
	}
}

void SpinBox_Init(UI_SpinBox_Struct* e)
{
	lv_obj_t* parent = e->parent;
	// set timer label
	lv_obj_t* label = lv_label_create(parent);
	lv_obj_t* spinbox = lv_spinbox_create(parent);

	char* Label_Str = e->Label_Str;
	
	lv_spinbox_set_value(spinbox, *e->SpinBox_Value);
	lv_spinbox_set_range(spinbox, 1, 1000);
	lv_spinbox_set_digit_format(spinbox, 4, 0);
	lv_spinbox_step_prev(spinbox);
	lv_obj_set_width(spinbox, 100);
	lv_obj_set_size(spinbox, 130, 42);

	lv_obj_align(spinbox, LV_ALIGN_LEFT_MID, e->x, e->y);
	lv_obj_align_to(label, spinbox, LV_ALIGN_OUT_LEFT_MID, -25, 0);
	lv_obj_clear_flag(spinbox, LV_OBJ_FLAG_SCROLLABLE);
	// binding spinbox Value
	spinbox->user_data = e->SpinBox_Value;
	if (e->SpinBox_Value != 0)
		lv_spinbox_set_value(spinbox, *e->SpinBox_Value);

	// label_Set
	if (Label_Str != 0)
		lv_label_set_text_fmt(label, Label_Str);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

	// set btn
	lv_obj_t* btn_decrease = lv_btn_create(parent);
	lv_obj_t* btn_increase = lv_btn_create(parent);

	lv_obj_set_size(btn_decrease, 40, 40);
	lv_obj_set_size(btn_increase, 40, 40);
	lv_obj_align_to(btn_decrease, spinbox, LV_ALIGN_OUT_RIGHT_MID, 65, 0);
	lv_obj_align_to(btn_increase, spinbox, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
	// set
	lv_obj_set_style_bg_img_src(btn_increase, LV_SYMBOL_PLUS, 0);
	lv_obj_set_style_bg_img_src(btn_decrease, LV_SYMBOL_MINUS, 0);
	btn_increase->user_data = (void*)SpinBox_increase;
	btn_decrease->user_data = (void*)SpinBox_decrease;
	// btn event
	lv_obj_add_event_cb(btn_increase, SpinBox_Btn_handler, LV_EVENT_ALL, (void*)spinbox);
	lv_obj_add_event_cb(btn_decrease, SpinBox_Btn_handler, LV_EVENT_ALL, (void*)spinbox);

	*e->SpinBox_Value = lv_spinbox_get_value(spinbox);
}
