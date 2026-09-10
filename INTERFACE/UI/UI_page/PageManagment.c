#include "PageManagment.h"

#include "../UI_Include.h"
#include "../../UI/other/meassage_center.h"
#include "page1.h"
#include "Page_Maintain.h"
#include "Page_Varify.h"
//#include "C610.h"
static lv_obj_t *top_layer;
lv_obj_t *UI_Page_Management_Get_Top_layer()
{
	return top_layer;
}

void Timer_Refresh_handle(lv_timer_t *t)
{
	//extern  motor_measure_t motor_chassis[7];
	lv_obj_t *label = t->user_data;
	static int i = 0;
	if (label != 0)
	{

		timetyped_UI time = {0};
		Message_Center_Read_prinft("Ctrl", &time, sizeof(time), "ReqTime");
		lv_label_set_text_fmt(label, "%04d/%d/%d\n\t%d:%02d:%02d",
							time.date.year, time.date.month, time.date.date,
							  time.date.hour, time.date.min, time.date.sec);
	}
	else
	{
		t->repeat_count = 0;
	}
}

void Timer_Refresh_Label_init(lv_obj_t *parent)
{
	lv_obj_t *label = lv_label_create(
		lv_scr_act());
	lv_label_set_text_fmt(label, "");
	//
	/*lv_obj_t* label = lv_obj_create(
		lv_scr_act());
	lv_obj_set_size(label, 700, 40);*/
	lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -15, 10);
	lv_timer_t *t = lv_timer_create(Timer_Refresh_handle, 1000, label);
	lv_timer_ready(t);
}
// 烧录
// 硬件配置（灯光 相机wifi 倾斜 陀螺仪 光电 错误历史 内存占用 ）
// 测试模组
void UI_Page_Management_Init()
{

	// lv_obj_t* parent = lv_scr_act();
	////Page1_init(parent);
	// lv_example_menu_32();

	/*Create a menu object*/
	top_layer = lv_obj_create(lv_scr_act());
	lv_obj_set_size(top_layer, LV_HOR_RES, LV_VER_RES);	  // 让它覆盖整个屏幕
	lv_obj_clear_flag(top_layer, LV_OBJ_FLAG_SCROLLABLE); // 取消滚动
	lv_obj_set_style_bg_opa(top_layer, LV_OPA_TRANSP, 0); // 透明背景

	lv_obj_t *menu = lv_menu_create(lv_scr_act());

	lv_obj_set_parent(menu, top_layer); // 把 obj 作为子对象

	lv_obj_set_size(menu, 800, 480);
	lv_obj_center(menu);
	lv_color_t bg_color = lv_obj_get_style_bg_color(menu, 0);
	lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 50), 0);

	/*Modify the header*/
	lv_obj_t *back_btn = lv_menu_get_main_header_back_btn(menu);
	lv_obj_t *back_btn_label = lv_label_create(back_btn);
	lv_label_set_text(back_btn_label, "Back");

	/*timer display*/

	lv_obj_t *cont;
	lv_obj_t *label;

	/*Create sub pages*/
	lv_obj_t *sub_1_page = lv_menu_page_create(menu, "");
	// lv_obj_set_size(sub_1_page, 800, 430);
	lv_obj_t *obj = lv_obj_create(sub_1_page);
	lv_obj_set_size(obj, 795, 430);
	Page1_init(obj);

	Timer_Refresh_Label_init(menu);

	/*cont = lv_menu_cont_create(sub_1_page);
	label = lv_label_create(cont);
	lv_label_set_text(label, "Hello, I am hiding here");*/

	lv_obj_t *sub_2_page = lv_menu_page_create(menu, "Settings");

	extern void lv_example_menu_55(lv_obj_t * parent, lv_obj_t * obj);
	lv_example_menu_55(sub_2_page, menu);

	/*lv_obj_t* sub_3_page = lv_menu_page_create(menu, "Page 3");

	cont = lv_menu_cont_create(sub_3_page);
	label = lv_label_create(cont);
	lv_label_set_text(label, "Hello, I am hiding here");*/

	lv_obj_t *setpageCont;
	/*Create a main page*/
	lv_obj_t *main_page = lv_menu_page_create(menu, "menu");
	cont = lv_menu_cont_create(main_page);
	label = lv_label_create(cont);
	lv_label_set_text(label, "->Running");
	lv_menu_set_load_page_event(menu, cont, sub_1_page);
	/*show running page*/
	setpageCont = cont;

	cont = lv_menu_cont_create(main_page);
	label = lv_label_create(cont);
	lv_label_set_text(label, "->Settings");
	lv_menu_set_load_page_event(menu, cont, sub_2_page);

	// cont = lv_menu_cont_create(main_page);
	// label = lv_label_create(cont);
	// lv_label_set_text(label, "->Item 3");
	////lv_menu_set_load_page_event(menu, cont, sub_3_page);

	lv_menu_set_page(menu, main_page);
	lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(sub_1_page, LV_OBJ_FLAG_SCROLLABLE);
	lv_event_send(setpageCont, LV_EVENT_CLICKED, 0);
	int value = 0;
	Message_Center_Read_prinft("Ctrl", &value, sizeof(int), "isNeedSetup");
	// value = 0;
//	Verify_hardResetVaild();
	value = 0;
#ifndef ATM32F43_xx
	//value = 1;
#endif
	switch (value)
	{
	case 1: // expired
		UI_Page_Management_Expored(top_layer, 1);
		break;
	case 3: // settime only
		UI_Page_Management_Expored(top_layer, 0);
		break;
	case 4: // setboth
		UI_Page_Management_Expored(top_layer, 2);
		break;
	}
	// if(value==)
	if (value == 2)
		;

	// lv_menu_setpage
}

static void keyboardHidden(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CANCEL)
	{
		lv_obj_add_flag(e->current_target, LV_OBJ_FLAG_HIDDEN);
	}
}

lv_obj_t *UI_get_KeyBorad(lv_obj_t *parent)
{
	static lv_obj_t *kb;
	if (kb == 0)
	{
		kb = lv_keyboard_create(parent);
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_event_cb(kb, keyboardHidden, LV_EVENT_CANCEL, 0);
	}
	// lv_obj_set_parent(kb, parent);
	lv_obj_move_foreground(kb);
	return kb;
}