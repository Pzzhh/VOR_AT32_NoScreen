#include "Page_Outage_Event.h"
#include "page1_message_id.h"
// #include "page_include.h"
#include "../../UI/page_include.h"
#include "../../UI/Task/task_manager.h"
#include "../../UI/UI_Component/UI_ListBox.h"
#include "../../UI/UI_Parameter/UI_Parameter_Set.h"
#include "../../UI/control/control.h"
#include "../../UI/control/control_Hardware_API.h"
#include "string.h"

static void msgboxD_event_handler(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* msgbox = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// 获取被点击的按钮索引
		uint16_t btn_id = lv_msgbox_get_active_btn(msgbox);
		lv_msgbox_close(msgbox);
		if (btn_id == 0) {
			uint8_t StartWithIndex = 1;
			Message_Center_Read_prinft("Ctrl", &StartWithIndex, sizeof(uint8_t), "ReadOutageIndex");
			Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Clean");
			Message_Center_Send_prinft("Ctrl", &StartWithIndex, sizeof(uint8_t), "ReqStratWithIndex");
		}
		else if (btn_id == 1)
		{
			Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Clean");
		}
	}
}

void UI_Page1_Outage_Notice_D()
{
	int res = 0;
	static const char* btns[] = { "Run","Skip", NULL };
	const char* format = "Ready To Run at Task No:%d\n";
	lv_obj_t* msgbox = lv_msgbox_create(NULL, " ", " ",
		btns, false);
	lv_obj_t* label = lv_msgbox_get_text(msgbox);
	uint8_t StartWithIndex = 0;
	Message_Center_Read_prinft("Ctrl", &StartWithIndex, sizeof(uint8_t), "ReadOutageIndex");
	lv_label_set_text_fmt(label, format,StartWithIndex+1);
	lv_obj_set_width(msgbox, 500);
	lv_obj_center(msgbox);
	lv_obj_add_event_cb(msgbox, msgboxD_event_handler, LV_EVENT_ALL, 0);
}

void UI_Page1_Outage_Notice_C_TimerHandler(lv_timer_t* t)
{
	uint8_t res = 0;
	Message_Center_Read_prinft("Ctrl", &res, sizeof(uint8_t), "IsInitFinish");
	if (res == 0)
	{
		lv_obj_t* msgbox = (lv_obj_t*)t->user_data;
		if (msgbox != 0)
			lv_msgbox_close(msgbox);
		lv_timer_del(t);
		UI_Page1_Outage_Notice_D();
		//加入启动提示
	}
}

static void msgboxC_event_handler(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* msgbox = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// 获取被点击的按钮索引
		uint16_t btn_id = lv_msgbox_get_active_btn(msgbox);
		lv_msgbox_close(msgbox);
		if (btn_id == 0) {
			// "Confirm" button clicked
			Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Clean");
			lv_timer_t* t = e->user_data;
			if (t != 0)
				lv_timer_del(t);
			lv_msgbox_close(msgbox);
		}
	}
}

void UI_Page1_Outage_Notice_C()
{
	int res = 0;
	static const char* btns[] = { "Quit", NULL };
	const char* format = "pending initialization done\n";
	lv_obj_t* msgbox = lv_msgbox_create(NULL, " ", " ",
		btns, false);
	lv_obj_t* label = lv_msgbox_get_text(msgbox);
	lv_label_set_text_fmt(label, format);
	lv_obj_set_width(msgbox, 500);
	lv_obj_center(msgbox);
	lv_timer_t* t = lv_timer_create(UI_Page1_Outage_Notice_C_TimerHandler, 1000, msgbox);
	lv_obj_add_event_cb(msgbox, msgboxC_event_handler, LV_EVENT_ALL, (void*)t);
}

static void msgboxB_event_handler(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* msgbox = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// 获取被点击的按钮索引
		uint16_t btn_id = lv_msgbox_get_active_btn(msgbox);
		lv_msgbox_close(msgbox);
		Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Resume");
		if (btn_id == 0) {
			Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Clean");
		}
		else if (btn_id == 1) {
			UI_Page1_Outage_Notice_C();
		}
	}
}

void UI_Page1_Outage_Notice_B()
{
	static const char* btns[] = { "A", "B", NULL };

	const char* format = "Option A: Just resume Last Task\n"
		"Option B: resume at Outage Task\n";
	lv_obj_t* msgbox = lv_msgbox_create(NULL, " ", " ",
		btns, false);
	lv_obj_t* label = lv_msgbox_get_text(msgbox);
	lv_label_set_text_fmt(label, format);
	lv_obj_set_width(msgbox, 500);
	lv_obj_center(msgbox);
	lv_obj_add_event_cb(msgbox, msgboxB_event_handler, LV_EVENT_ALL, NULL);
}


static void msgbox_event_handler(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* msgbox = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// 获取被点击的按钮索引
		uint16_t btn_id = lv_msgbox_get_active_btn(msgbox);
		lv_msgbox_close(msgbox);
		if (btn_id == 0) {
			// "Confirm" button clicked
			//Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Resume");
			UI_Page1_Outage_Notice_B();
		}
		else if (btn_id == 1) {
			// "Cancel" button clicked
			Message_Center_Read_prinft("Ctrl", 0, 0, "Outage Clean");
		}
	}
}



void UI_Page1_Outage_Notice()
{
	uint8_t res = 0;
	Message_Center_Read_prinft("Ctrl", &res, sizeof(res), "isOutage?");
	if (res == 0)
		return;
	static const char* btns[] = { "Resume", "Ignore", NULL };

	const char* format = "Last task was interrupted due to a power outage.\n"
		"Do you want to resume?\n"
		"Task At""%04d/%d/%d %d:%02d:%02d";
	timetyped time = { 0 };
	Message_Center_Read_prinft("Ctrl", &time, sizeof(time), "OutageTime?");
	lv_obj_t* msgbox = lv_msgbox_create(NULL, "Outage Event", " ",
		btns, false);
	lv_obj_t* label = lv_msgbox_get_text(msgbox);
	lv_label_set_text_fmt(label, format, time.date.year, time.date.month, time.date.date,
		time.date.hour, time.date.min, time.date.sec);
	lv_obj_set_width(msgbox, 500);
	lv_obj_center(msgbox);
	lv_obj_add_event_cb(msgbox, msgbox_event_handler, LV_EVENT_ALL, NULL);
}
