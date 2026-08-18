#include "Page_Maintain.h"
#include "../UI_Include.h"
#include "../other/meassage_center.h"
#include "Page_Varify.h"

enum
{
	LV_MENU_ITEM_BUILDER_VARIANT_1,
	LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;

extern lv_obj_t *UI_Page_Management_Get_Top_layer();

static void back_event_handler(lv_event_t *e);
// static void switch_handler(lv_event_t* e);
lv_obj_t *root_page;
static lv_obj_t *create_text(lv_obj_t *parent, const char *icon, const char *txt,
							 lv_menu_builder_variant_t builder_variant);
static lv_obj_t *create_slider(lv_obj_t *parent,
							   const char *icon, const char *txt, int32_t min, int32_t max, int32_t val);
static lv_obj_t *create_switch(lv_obj_t *parent,
							   const char *icon, const char *txt, bool chk);

/*HANDLE CAM*/ static void Camera_Rec_sw_handle(lv_event_t *e)
{
	lv_obj_t *sw = e->current_target;
	uint8_t isclick = lv_obj_has_state(sw, LV_STATE_CHECKED);
	if (Message_Center_Read_prinft("Ctrl", 0, 0,
								   "Camere Rec", !isclick) == 0)
	{
		if (isclick)
			lv_obj_clear_state(sw, LV_STATE_CHECKED);
		else
			lv_obj_add_state(sw, LV_STATE_CHECKED);
	}
}

/*HANDLE CAM*/ static void Camera_WIFI_btn_handle(lv_event_t *e)
{
	lv_obj_t *btn = e->current_target;
	char msg[10] = {0};
	uint8_t state = 1;
	if (Message_Center_Read_prinft("Ctrl", msg, sizeof(msg),
								   "Camere wifi") == 0)
	{
		state = 0;
		if (strncmp(msg, "Fail", 4) == 0)
		{
			state = 2;
		}
	}
	if (state != 0)
	{
		lv_obj_t *mbox1 = lv_msgbox_create(NULL, "error message",
										   (state == 1) ? "CMD FAIL" : (state == 2) ? "CAN connect FAIL"
																					: "Reserved",
										   0, true);
		lv_obj_center(mbox1);
	}
}

/*HANDLE CAM*/ static void Camera_vol_slider_handle(lv_event_t *e)
{
	const uint32_t offsetVal = 1000;
	lv_obj_t *slider = e->current_target;
	char msg[20] = {0};
	uint8_t state = 1;
	uint32_t sliderVal = lv_slider_get_value(slider);
	uint32_t VolSet = sliderVal * 5 + 1000;
	lv_label_set_text_fmt(slider->user_data, "%01d.%03d", VolSet / 1000, VolSet % 1000);
	if (Message_Center_Read_prinft("Ctrl", msg, sizeof(msg),
								   "Camere Vol %d", VolSet) == 0)
	{
		lv_obj_t *mbox1 = lv_msgbox_create(NULL, "error message",msg,0, true);
		lv_obj_center(mbox1);
//		state = 0;
//		if (strlen(msg) > 0)
//		{
//			state = 1;
//		}
	}
	if (state == 1)
	{
//		lv_obj_t *mbox1 = lv_msgbox_create(NULL, "error message",
//										   (state == 1) ? "CMD FAIL" : (state == 2) ? "CAN connect FAIL"
//																					: "Reserved",
//										   0, true);
//		lv_obj_center(mbox1);
	}
}

/*HANDLE FAN*/ static void FAN_vol_slider_handle(lv_event_t *e)
{
	lv_obj_t *slider = e->current_target;
	uint32_t sliderVal = lv_slider_get_value(slider);
	uint32_t VolSet = sliderVal;
	lv_label_set_text_fmt(slider->user_data, "%d", VolSet);
	if (Message_Center_Read_prinft("Ctrl", 0, 0,
								   "FAN_pwm %d", VolSet) == 0)
	{
	}
}

void UI_page_maintain_incline_Up_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_PRESSING)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "Inc Up");
	}
	if (e->code == LV_EVENT_RELEASED)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "Inc stop");
	}
}

void UI_page_maintain_incline_down_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_PRESSING)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "Inc down");
		// printf("...");
	}
	if (e->code == LV_EVENT_RELEASED)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "Inc stop");
		// printf("|");
	}
}

void UI_page_maintain_SYSINFO_hander(lv_event_t *e)
{
	static int Password = 122112;
	static int Input = 0;
	static uint8_t isLongPress = 0;
	extern void UI_page_maintain_DEV_Menu_Init();
	if (e->code != LV_EVENT_CLICKED && e->code != LV_EVENT_LONG_PRESSED)
		return;
#if 1
	UI_page_maintain_DEV_Menu_Init();
#else
	lv_obj_t *btn = e->user_data;
	lv_obj_t *label = lv_obj_get_child(btn, 0);
	if (e->code == LV_EVENT_CLICKED)
	{
		if (isLongPress == 0)
		{
			if (Input == 0)
				Input = 1;
			else
				Input = Input * 10 + 1;
		}
		isLongPress = 0;
	}
	if (e->code == LV_EVENT_LONG_PRESSED)
	{
		if (Input == 0)
		{
			Input = 2;
		}
		else
		{
			Input = Input * 10 + 2;
		}
		isLongPress = 1;
	}
	if (Input >= Password)
	{
		if (Input == Password)
			UI_page_maintain_DEV_Menu_Init();
		char str[30] = {0};
		HAL_GET_SYSINFO(str);
		lv_label_set_text(label, str);
		Input = 0;
	}
	else
	{
		if (Input != 0)
			lv_label_set_text_fmt(label, "PWD:%d", Input);
	}
#endif
}

void UI_page_maintain_DEV_hander(lv_event_t *e)
{
	if (e->code == LV_EVENT_VALUE_CHANGED)
	{
		lv_obj_t *obj = lv_event_get_target(e);
		int value = lv_obj_has_state(obj, LV_STATE_CHECKED);
		const char *cmd = e->user_data;
		if (value)
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
									   "DEV %s 1", cmd);
		}
		else
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
									   "DEV %s 0", cmd);
		}
	}
}

void UI_page_maintain_C610_hander(lv_event_t *e)
{
	if (e->code == LV_EVENT_VALUE_CHANGED)
	{
		lv_obj_t *obj = lv_event_get_target(e);
		int value = lv_obj_has_state(obj, LV_STATE_CHECKED);
		const char *cmd = e->user_data;
		if (value)
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
									   "DEV %s 1", cmd);
		}
		else
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
									   "DEV %s 0", cmd);
		}
	}
}

void UI_page_maintain_UART_hander(lv_event_t *e)
{
	if (e->code == LV_EVENT_VALUE_CHANGED)
	{
		const char *cmd = e->user_data;
		int options = lv_dropdown_get_selected(e->current_target);
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "DEV %s %d", cmd, options);
	}
}

lv_obj_t *UI_page_COMBOX_init(lv_obj_t *parent, const char *Items, lv_event_cb_t event_cb)
{
	lv_obj_t *obj = parent;
	// lv_label_t label* lv_label_create(parent);
	lv_obj_t *combox = lv_dropdown_create(obj);
	lv_obj_set_width(combox, 65);
	lv_obj_align(combox, LV_ALIGN_LEFT_MID, 0, 0);
	lv_dropdown_set_options(combox, Items);
	lv_obj_set_style_bg_color(combox, lv_color_hex(0x696969), LV_PART_INDICATOR | LV_STATE_CHECKED);
	int value = 0;
	lv_dropdown_set_selected(combox, value);
	if (event_cb != 0)
	{
		lv_obj_add_event_cb(combox, event_cb, LV_EVENT_VALUE_CHANGED, 0);
	}
	return combox;
}

void UI_page_maintain_Sw_init(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb, const char *cmd)
{
	lv_obj_t *obj = lv_obj_create(parent);
	lv_obj_set_size(obj, 300, 50);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *sw = lv_switch_create(obj);
	lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_t *label = lv_label_create(obj);
	lv_label_set_text(label, text);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_set_style_bg_color(sw, lv_color_hex(0x696969), LV_PART_INDICATOR | LV_STATE_CHECKED);
	int value = 0;
	Message_Center_Read_prinft("Ctrl", &value, sizeof(value),
							   "DEV %s", cmd);
	if (value)
		lv_obj_add_state(sw, LV_STATE_CHECKED);
	if (event_cb != 0)
	{
		lv_obj_add_event_cb(sw, event_cb, LV_EVENT_VALUE_CHANGED, (void *)cmd);
	}
}

void UI_page_maintain_COMBOX_init(lv_obj_t *parent, const char *text, const char *Items, lv_event_cb_t event_cb, const char *cmd)
{
	lv_obj_t *obj = lv_obj_create(parent);
	lv_obj_set_size(obj, 300, 50);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *combox = lv_dropdown_create(obj);
	lv_obj_align(combox, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_t *label = lv_label_create(obj);
	lv_label_set_text(label, text);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	lv_dropdown_set_options(combox, Items);
	lv_obj_set_style_bg_color(combox, lv_color_hex(0x696969), LV_PART_INDICATOR | LV_STATE_CHECKED);
	int value = 0;
	Message_Center_Read_prinft("Ctrl", &value, sizeof(value),
							   "DEV %s", cmd);
	lv_dropdown_set_selected(combox, value);
	if (event_cb != 0)
	{
		lv_obj_add_event_cb(combox, event_cb, LV_EVENT_VALUE_CHANGED, (void *)cmd);
	}
}

void UI_page_maintain_label_init(lv_obj_t *parent, const char *text, const char *Items, lv_event_cb_t event_cb)
{
}

#include "../../UI/UI_Component/UI_SpinBox.h"
void UI_page_maintain_Num_init(lv_obj_t *parent, const char *text, const char *Items, lv_event_cb_t event_cb, const char *cmd)
{
	static int num;
	uint64_t pt = 0;
	Message_Center_Read_prinft("Ctrl", &pt, sizeof(pt),
							   "GET C610 Factor");
	if (pt == 0)
	{
		pt = (uint64_t)&num;
	}
	lv_obj_t *obj = lv_obj_create(parent);
	lv_obj_set_size(obj, 300, 50);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *label = 0, *spinbox = 0;
	UI_SpinBox_Struct e = {
		.parent = obj,
		.label = label,
		.spinbox = spinbox,
		.SpinBox_Value = (int32_t *)pt,
		.x = 45,
		.y = -0,
		.Label_Str = "c610"};
	SpinBox_Init(&e);
	/*lv_obj_t* combox = lv_dropdown_create(obj);
	lv_obj_align(combox, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_t* label = lv_label_create(obj);
	lv_label_set_text(label, text);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	lv_dropdown_set_options(combox, Items);
	lv_obj_set_style_bg_color(combox, lv_color_hex(0x696969), LV_PART_INDICATOR | LV_STATE_CHECKED);
	int value = 0;
	Message_Center_Read_prinft("Ctrl", &value, sizeof(value),
		"DEV %s", cmd);
	lv_dropdown_set_selected(combox, value);
	if (event_cb != 0)
	{
		lv_obj_add_event_cb(combox, event_cb, LV_EVENT_VALUE_CHANGED, (void*)cmd);
	}*/
}

void UI_page_maintain_time_btn_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_CLICKED)
	{
		extern void UI_Maintain_Time_Set();
		UI_Maintain_Time_Set();
	}
}

void UI_page_maintain_DFU_btn_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_CLICKED)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "DFU");
	}
}

void UI_page_maintain_Uart_Test_btn_handle(lv_event_t *e)
{
	uint32_t res = 0;
	const char displayLabel[][5] = {
		"N|N",
		"N|P",
		"P|N",
		"P|P",
	}; // 0 nn 01 np 10 pn 11 pp
	if (e->code == LV_EVENT_CLICKED)
	{
		Message_Center_Read_prinft("Ctrl", &res, sizeof(res),
								   "UART Test");
		lv_obj_t *parent = (lv_obj_t *)e->user_data;
		if (parent != 0)
		{
			lv_obj_t *label = lv_obj_get_child(parent, 0);
			if (res >= 4)
				res = 0;
			lv_label_set_text_fmt(label, "UART TEST %s", displayLabel[res]);
		}
	}
}

void Encode_reflush_Timer(lv_timer_t *e)
{
	int32_t res = 0;
	Message_Center_Read_prinft("Ctrl", &res, sizeof(res),
							   "ENCODE Test");
	lv_obj_t *parent = (lv_obj_t *)e->user_data;
	if (parent != 0)
	{
		lv_obj_t *label = lv_obj_get_child(parent, 0);
		lv_label_set_text_fmt(label, "ENCODE T %d", res);
	}
}

void UI_page_maintain_Encoder_btn_handle(lv_event_t *e)
{
	static lv_timer_t *timer;

	// const char displayLabel[][5] = { "N|N","N|P","P|N","P|P", };//0 nn 01 np 10 pn 11 pp
	if (e->code == LV_EVENT_CLICKED)
	{
		if (timer == 0)
			timer = lv_timer_create(Encode_reflush_Timer, 100, e->user_data);
	}
	if (e->code == LV_EVENT_DELETE)
	{
		if (timer != 0)
			lv_timer_del(timer);
		timer = 0;
	}
}

void UI_page_maintain_license_btn_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_CLICKED)
	{
		timetyped_UI time = {0};
		Message_Center_Read_prinft("Ctrl", &time, sizeof(time),
								   "GetLicense");
		char label[50];
		sprintf(label, "%04d/%d/%d\n\t",
				time.date.year, time.date.month, time.date.date);

		lv_obj_center(lv_msgbox_create(NULL, "License", label, NULL, true));
	}
}

void UI_page_maintain_Factory_Reset_btn_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_CLICKED)
	{
		timetyped_UI time = {0};
		Message_Center_Read_prinft("Ctrl", &time, sizeof(time),
								   "FACTORY Reset");
	}
}

void UI_page_maintain_Settime_btn_handle(lv_event_t *e)
{
	lv_obj_t *parent = (lv_obj_t *)e->user_data;
	if (e->code == LV_EVENT_CLICKED)
	{
		lv_msgbox_close(parent);
		if (parent != 0)
			UI_Page_Management_Expored(UI_Page_Management_Get_Top_layer(), 2);
	}
}

void UI_page_maintain_MsgBox_close(lv_event_t *e)
{
	if (e->code == LV_EVENT_DELETE)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "CONFIG SAVE");
	}
}

void UI_page_maintain_DEV_Menu_Init()
{

	lv_obj_t *obj = lv_msgbox_create(NULL, "DEV MODE", " ", NULL, true);
	lv_obj_center(obj);
	lv_obj_set_size(obj, 400, 400);
	UI_page_maintain_Sw_init(obj, "LOOP", UI_page_maintain_DEV_hander, "LOOP");
	UI_page_maintain_Sw_init(obj, "C610", UI_page_maintain_C610_hander, "C610 CONFIG");
	UI_page_maintain_COMBOX_init(obj, "UART", "DISABLE\nUART1\nBoth", UI_page_maintain_UART_hander, "UART CONFIG");
	lv_obj_t *btn = lv_list_add_btn(obj, 0, "DFU");
	lv_obj_t *btn_Encode_TEST = lv_list_add_btn(obj, 0, "ENCODE");
	lv_obj_t *btn_UART_TEST = lv_list_add_btn(obj, 0, "UART TEST");
	lv_obj_t *btn_display_license = lv_list_add_btn(obj, 0, "license");
	lv_obj_t *btn_Factory_Reset = lv_list_add_btn(obj, 0, "Factory Reset");
	UI_page_maintain_Num_init(obj, "sec", "sss", NULL, "c");
	lv_obj_add_event_cb(btn, UI_page_maintain_DFU_btn_handle, LV_EVENT_CLICKED, btn);
	lv_obj_add_event_cb(btn_UART_TEST, UI_page_maintain_Uart_Test_btn_handle, LV_EVENT_CLICKED, btn_UART_TEST);
	lv_obj_add_event_cb(btn_Encode_TEST, UI_page_maintain_Encoder_btn_handle, LV_EVENT_ALL, btn_Encode_TEST);
	lv_obj_add_event_cb(btn_display_license, UI_page_maintain_license_btn_handle, LV_EVENT_CLICKED, btn);
	lv_obj_add_event_cb(btn_Factory_Reset, UI_page_maintain_Factory_Reset_btn_handle, LV_EVENT_CLICKED, 0);
	lv_obj_add_event_cb(obj, UI_page_maintain_MsgBox_close, LV_EVENT_DELETE, 0);
}

void UI_page_maintain_incline_init(lv_obj_t *parent)
{
	lv_obj_t *btn = lv_btn_create(parent);
	lv_obj_t *label = lv_label_create(btn);
	lv_label_set_text(label, LV_SYMBOL_UP);

	lv_obj_add_event_cb(btn, UI_page_maintain_incline_Up_handle, LV_EVENT_ALL, 0);

	btn = lv_btn_create(parent);
	label = lv_label_create(btn);
	lv_label_set_text(label, LV_SYMBOL_DOWN);

	lv_obj_add_event_cb(btn, UI_page_maintain_incline_down_handle, LV_EVENT_ALL, 0);
}

void UI_page_task_Ctrl_Save_handle(lv_event_t *e)
{
	extern int Task_Stroage_GetSize();
	if (e->code == LV_EVENT_RELEASED)
	{
		if (Task_Stroage_GetSize() == 0)
		{
			lv_obj_t *obj = lv_msgbox_create(NULL, "info", "Please add a task before saving", NULL, true);
			lv_obj_center(obj);
			return;
		}
		lv_obj_t *combox = (lv_obj_t *)e->user_data;
		int index = 0;
		if (combox != 0)
		{
			index = lv_dropdown_get_selected(combox);
		}
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "task save %d", index);
		lv_obj_t *obj = lv_msgbox_create(NULL, "info", "Config Saved", NULL, true);
		lv_obj_center(obj);
	}
}

void UI_page_task_Ctrl_Load_handle(lv_event_t *e)
{
	if (e->code == LV_EVENT_RELEASED)
	{
		lv_obj_t *combox = (lv_obj_t *)e->user_data;
		int index = 0;
		if (combox != 0)
		{
			index = lv_dropdown_get_selected(combox);
		}
		uint8_t res = 0;
		Message_Center_Read_prinft("Ctrl", &res, sizeof(uint8_t),
								   "task load %d", index);
		lv_obj_t *obj = lv_msgbox_create(NULL, "info", (res == 0) ? "Config Empty" : "Config Loaded", NULL, true);
		lv_obj_center(obj);
	}
}

void UI_page_task_Ctrl_init(lv_obj_t *parent)
{
	lv_obj_t *combox = UI_page_COMBOX_init(parent, "1\n"
												   "2\n"
												   "3",
										   0);
	lv_obj_t *btn = lv_btn_create(parent);
	lv_obj_t *label = lv_label_create(btn);
	lv_label_set_text(label, "save");
	lv_obj_add_event_cb(btn, UI_page_task_Ctrl_Save_handle, LV_EVENT_ALL, (void *)combox);

	btn = lv_btn_create(parent);
	label = lv_label_create(btn);
	lv_label_set_text(label, "load");

	lv_obj_add_event_cb(btn, UI_page_task_Ctrl_Load_handle, LV_EVENT_ALL, (void *)combox);
}

static void anim_y_cb(void *var, int32_t v)
{

	lv_obj_t *label = var;
	if (lv_obj_is_visible(var))
	{
		float angle = 0;
		HAL_IMU_GET_Angle(&angle);
		float factor = (angle >= 0) ? angle : -angle;
		lv_label_set_text_fmt(label, "%d.%02d", (int)angle, (int)(factor * 100.0f) % 100);
	}
	// lv_obj_set_y((lv_obj_t*)var, v);
}

static void ta_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *ta = lv_event_get_target(e);
	lv_obj_t *kb = lv_event_get_user_data(e);
	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED)
	{
		lv_keyboard_set_textarea(kb, ta);
		lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	}
	if (code == LV_EVENT_VALUE_CHANGED)
	{

		/*	strncpy(MouseNameTextArea_textSource,
				lv_textarea_get_text(e->current_target),
				50);*/
		int value[6];
		int res = 0;
		const char *str = lv_textarea_get_text(e->current_target);
		int len = strlen(str);
		printf("\r\n len %d", len);
		if (strlen(str) == 19)
		{
			res = sscanf(str, "%d/%d/%d/%d/%d/%d",
						 &value[0], &value[1], &value[2],
						 &value[3], &value[4], &value[5]);
			Message_Center_Read_prinft("Ctrl", 0, 0,
									   "DEV TIME %s", str);
		}

		if (res == 6)
		{
			res = 0;
		}
	}

	if (code == LV_EVENT_DEFOCUSED)
	{
		lv_keyboard_set_textarea(kb, NULL);
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	}
}

void UI_Maintain_Time_Set(lv_obj_t *parent)
{
	lv_obj_t *obj = lv_msgbox_create(NULL, "Time Set", " ", NULL, true);
	lv_obj_set_size(obj, 700, 500);
	lv_obj_center(obj);
	/*Create a keyboard to use it with an of the text areas*/

	// lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *label = lv_label_create(obj);
	lv_label_set_text(label, "2000/01/01/10/12/50\nYYYY/MM/DD/hh/mm/ss");

	/*Create a text area. The keyboard will write here*/
	lv_obj_t *ta;
	ta = lv_textarea_create(obj);
	lv_obj_align(ta, LV_ALIGN_TOP_LEFT, -0, 120);

	// lv_textarea_set_placeholder_text(ta, "2000/01/01/10/12/50 YYYY/MM/DD/hh/mm/ss");
	lv_textarea_set_accepted_chars(ta, "1234567890/");
	lv_textarea_set_max_length(ta, 19);
	// lv_obj_add_event_cb(kb, keyboardHidden, LV_EVENT_CANCEL, 0);
	lv_obj_set_size(ta, 240, 45);
	lv_obj_t *kb = lv_keyboard_create(obj);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, kb);
	lv_keyboard_set_textarea(kb, ta);
}

void lv_example_menu_55(lv_obj_t *parent, lv_obj_t *obj)
{
	// extern void UI_page_maintain_DEV_Menu_Init();
	// UI_page_maintain_DEV_Menu_Init();
	// UI_page_maintain_DEV_Menu_Init();
	// create_time_setting_screen();
	/*Create a list*/
	lv_obj_t *list1 = lv_list_create(parent);
	// lv_obj_clear_flag(list1, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(list1, 400, 400);
	lv_obj_center(list1);

	/*Add buttons to the list*/
	lv_obj_t *btn;

	lv_list_add_text(list1, "Camera");
	// lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_btn(list1, LV_SYMBOL_SETTINGS, "Rec");
	lv_obj_add_event_cb(btn,
						Camera_Rec_sw_handle, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_btn(list1, LV_SYMBOL_WIFI, "pair");
	lv_obj_add_event_cb(btn,
						Camera_WIFI_btn_handle, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_btn(list1, LV_SYMBOL_CHARGE, "IR led voltage\n");
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_t *silder = create_slider(btn, 0, 0, 0, 80, 50);
	lv_obj_add_event_cb(silder,
						Camera_vol_slider_handle, LV_EVENT_VALUE_CHANGED, NULL);
	// lv_event_send(silder, LV_EVENT_VALUE_CHANGED, NULL);

	lv_list_add_text(list1, "Task");
	btn = lv_list_add_btn(list1, NULL, "Profile");
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	UI_page_task_Ctrl_init(btn);

	lv_list_add_text(list1, "incline");
	btn = lv_list_add_btn(list1, LV_SYMBOL_SETTINGS, "manual control");
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	UI_page_maintain_incline_init(btn);
	// lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_btn(list1, LV_SYMBOL_GPS, "imu data");
	lv_obj_t *label = lv_label_create(btn);
	lv_anim_t a, *anim;
	lv_anim_init(&a);
	// a.user_data = table_proprity;
	lv_anim_set_var(&a, label);
	lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_exec_cb(&a, anim_y_cb); // 设置执行回调函数
	// lv_anim_set_path_cb(&a, custom_anim_path); // 设置自定义路径回调函数
	anim = lv_anim_start(&a); // 开始动画
	// lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);

	// lv_list_add_text(list1, "motion");
	// btn = lv_list_add_btn(list1, LV_SYMBOL_SETTINGS, "IR ");
	// // lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
	// btn = lv_list_add_btn(list1, LV_SYMBOL_SETTINGS, "servo ctrl");

	lv_list_add_text(list1, "other");
	btn = lv_list_add_btn(list1, 0, "FAN ");
	silder = create_slider(btn, 0, 0, 0, 100, 50);
	lv_obj_add_event_cb(silder,
						FAN_vol_slider_handle, LV_EVENT_VALUE_CHANGED, NULL);
	// lv_event_send(silder, LV_EVENT_VALUE_CHANGED, NULL);

	static char snstr[30];
	HAL_GET_SYSINFO(snstr);
	btn = lv_list_add_btn(list1, 0, snstr);
	lv_obj_add_event_cb(btn, UI_page_maintain_SYSINFO_hander, LV_EVENT_ALL, btn);
	// lv_list_add_text(list1, "date");
	// btn = lv_list_add_btn(list1, LV_SYMBOL_OK, "time");
}

static void back_event_handler(lv_event_t *e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	lv_obj_t *menu = lv_event_get_user_data(e);

	if (lv_menu_back_btn_is_root(menu, obj))
	{
		lv_obj_t *mbox1 = lv_msgbox_create(NULL, "Hello", "Root back btn click.", NULL, true);
		lv_obj_center(mbox1);
	}
}

static lv_obj_t *create_text(lv_obj_t *parent, const char *icon, const char *txt,
							 lv_menu_builder_variant_t builder_variant)
{
	lv_obj_t *obj = (lv_obj_t *)lv_menu_cont_create(parent);

	lv_obj_t *img = NULL;
	lv_obj_t *label = NULL;

	if (icon)
	{
		img = lv_img_create(obj);
		lv_img_set_src(img, icon);
	}

	if (txt)
	{
		label = lv_label_create(obj);
		lv_label_set_text(label, txt);
		lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
		lv_obj_set_flex_grow(label, 1);
	}

	if (builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt)
	{
		lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
		lv_obj_swap(img, label);
	}

	return obj;
}

static lv_obj_t *create_slider(lv_obj_t *parent, const char *icon, const char *txt, int32_t min, int32_t max,
							   int32_t val)
{
	// lv_obj_t *obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);
	lv_obj_t *slider = lv_slider_create(parent);
	slider->user_data = lv_label_create(parent);
	lv_label_set_text(slider->user_data, " ");
	// lv_obj_move_to(slider, 0, 50);
	// lv_obj_set_flex_grow(slider, 1);
	lv_slider_set_range(slider, min, max);
	lv_slider_set_value(slider, val, LV_ANIM_OFF);

	if (icon == NULL)
	{
		lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
	}

	return slider;
}

static lv_obj_t *create_switch(lv_obj_t *parent, const char *icon, const char *txt, bool chk)
{
	lv_obj_t *obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

	lv_obj_t *sw = lv_switch_create(obj);
	lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

	return obj;
}
