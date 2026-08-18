#include "../UI_Include.h"
#include "../../UI/other/meassage_center.h"
#include "Page_Varify.h"
#include "math.h"
#include "../control/control_Hardware_API.h"
#include "../UI_Component/UI_Dropdown.h"
#include "../UI_Component/UI_KeyBorad.h"
#define Default_str_Len 20
typedef struct
{
	struct
	{
		lv_obj_t* label1;
		lv_obj_t* dropdown1;

		UI_Dropdown_Struct obj;
	} Mouth;
	char unlock_input_str[Default_str_Len];
	uint8_t UnlockCode[9];
	lv_obj_t* label_UnlockCode;
	lv_obj_t* submit_btn_label;
	lv_obj_t* submit_btn;
	lv_obj_t* Msg_Box;
	lv_obj_t* text_Box;
	lv_obj_t* time_set_text_Box;
	lv_obj_t* qrcode;
} UI_Page_Verfiy;
static UI_Page_Verfiy Verfiy_Msg;
static const char mouth_to_expend[] = "1\n"
"3\n"
"6\n"
"12\n"
"24\n"
"36\n"
"48\n"
"60\n"
"255";
const int mouth_to_expend_int[] = { 1, 3, 6, 12, 24, 36, 48, 60, 255 };
static lv_obj_t* checkbox_toplayer_Parent = 0;

static timetyped timeReady2Write;
static uint8_t Setuptag = 0;
typedef enum
{
	timeOnly,
	reSubscribeOnly,
	Both,
	Both_C,
} SetupTyped;

void UI_Page_Verfiy_Comp(lv_obj_t* parent);

void UI_Gen_Unlock_Code(char str[50], uint8_t mouth)
{
	uint64_t uuid = HAL_GET_SYS_UUID();
	uint8_t UnlockCode[9];
	for (int i = 0; i < 1; i++)
	{
		UnlockCode[i] = HAL_Random_Create();
	}
	for (int i = 1; i < (1 + 2); i++)
	{
		UnlockCode[i] = uuid & 0xff;
		uuid = (uuid >> 8);
	}
	UnlockCode[4] = mouth;
	UnlockCode[5] = HAL_Random_Create();
	memcpy(Verfiy_Msg.UnlockCode, UnlockCode, 6);
	int pos = 0, len = 0;
	for (int i = 0; i < 6; i++)
	{
		len = sprintf(&str[pos], "%02x ", UnlockCode[i]);
		if (len > 0)
		{
			pos += (len);
			// pos--;
		}
	}
}

void UI_Page_Management_Expored_Deinit()
{
	extern lv_obj_t* UI_get_KeyBorad(void);
	lv_obj_t* kb = UI_get_KeyBorad();
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	del_custom_keyboard();
	lv_obj_del(Verfiy_Msg.Msg_Box);
}

static void ta_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* ta = lv_event_get_target(e);
	lv_obj_t* kb = get_custom_keyboard();
	uint8_t* str = (uint8_t*)Verfiy_Msg.unlock_input_str;
	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED)
	{
		lv_keyboard_set_textarea(kb, ta);
		lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	}
	if (code == LV_EVENT_VALUE_CHANGED)
	{
		char* instr = (char*)lv_textarea_get_text(e->current_target);
		int len = strlen(instr) + 1;
		if (str != 0)
			strncpy(str, instr, len);
	}

	if (code == LV_EVENT_DEFOCUSED)
	{
		lv_keyboard_set_textarea(kb, NULL);
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		// lv_obj_invalidate(kb);
	}
}

static void Check_Unlock_Code(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	uint8_t res = 0;

	if (strlen(Verfiy_Msg.unlock_input_str) == 6)
	{
		unsigned int value = { 0 }, month = 0;
		uint16_t crcfactor = Verfiy_Msg.UnlockCode[5];
		crcfactor = crcfactor * 256 + 0xf0;
		sscanf(Verfiy_Msg.unlock_input_str, "%06x", &value);
		month = value & 0xff;
		value = value >> 8;
		Verfiy_Msg.unlock_input_str[0] = 0;
		extern uint16_t crc16_Custom(const uint8_t * data, uint16_t length, uint16_t crcfactor);
		uint16_t crc = crc16_Custom(Verfiy_Msg.UnlockCode, 5, crcfactor);
		uint8_t supcode = 0;
		/*	if ((timeReady2Write.date.min) % 8 == 0)
			{
				uint8_t checka = timeReady2Write.date.min == (value >> 8);
				uint8_t checkb = (value & 0xFF) == 0xAB;
				if (checka && checkb)
				{
					value = crc;
				}
			}*/
		if (crc == value)
		{
			lv_event_code_t code = lv_event_get_code(e);
			uint8_t _month = month;
			// uint8_t month = Verfiy_Msg.UnlockCode[4];
			if (Setuptag == Both)
			{
				if (timeReady2Write.date.year != 0 || timeReady2Write.date.month != 0)
					Message_Center_Send_prinft("Ctrl", &timeReady2Write, sizeof(timeReady2Write), "SetTime");
				Message_Center_Send_prinft("Ctrl", &_month, sizeof(_month), "SetVaildTime");
				// cal
			}
			else if (Setuptag == reSubscribeOnly)
			{
				Message_Center_Send_prinft("Ctrl", &_month, sizeof(_month), "SetVaildTime");
			}
			res = 1;
			UI_Page_Management_Expored_Deinit();
		}
	}
	if (res == 0)
	{
		lv_textarea_set_text(Verfiy_Msg.text_Box, "");
		lv_textarea_set_placeholder_text(Verfiy_Msg.text_Box, "INCORRECT CODE PROVIDED");
	}
}

uint8_t TimeCheck(timetyped* time, int value[6])
{
	uint8_t rflag = 0;
	uint8_t res = 1;
	const int dayN[] = { 0, 31, 29, 31, 30, 31, 30,
						31, 31, 30, 31, 30, 31 };
	const int dayR[] = { 0, 31, 28, 31, 30, 31, 30,
						31, 31, 30, 31, 30, 31 };
	time->date.year = (value[0] > 50) ? 0 : value[0];
	if (time->date.year % 4 == 0)
		rflag = 1;
	const int* day = (rflag) ? dayR : dayN;
	time->date.month = (value[1] > 12 || value[1] == 0) ? 0 : value[1];
	time->date.date = (value[2] > day[time->date.month]) ? 0 : value[2];
	time->date.hour = (value[3] > 23) ? 0xff : value[3];
	time->date.min = (value[4] > 60) ? 0xff : value[4];
	for (int i = 0; i < 3; i++)
	{
		if (time->num[i] == 0)
		{
			res = 0;
		}
	}
	for (int i = 3; i < 5; i++)
	{
		if (time->num[i] == 0xff)
		{
			res = 0;
		}
	}
	return res;
}

static void Check_Time_Input(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	uint8_t res = 0;
	lv_obj_t* msgbox = e->user_data;
	if (1)
	{
		timetyped time = { 0 };
		int value[6] = { 0 };
		int count = sscanf(Verfiy_Msg.unlock_input_str, "%02d,%02d,%02d,%02d,%02d",
			&value[0], &value[1], &value[2], &value[3], &value[4]);
		Verfiy_Msg.unlock_input_str[0] = 0;
		if (count == 5)
		{
			res = TimeCheck(&time, value);
			if (res)
			{
				memcpy(&timeReady2Write, &time, sizeof(timetyped));
			}
		}
	}
	if (res == 0)
	{
		lv_textarea_set_text(Verfiy_Msg.time_set_text_Box, "");
		lv_textarea_set_placeholder_text(Verfiy_Msg.time_set_text_Box, "YY,MM,DD,HH,MM check format");
	}
}

static void TimeBox_Close(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (checkbox_toplayer_Parent)
	{
		if (Setuptag == Both)
			UI_Page_Verfiy_Comp(checkbox_toplayer_Parent);
	}
	if (Setuptag == timeOnly)
	{
		Message_Center_Send_prinft("Ctrl", &timeReady2Write, sizeof(timeReady2Write), "SetTime");
	}
}

static void dropdown_event_cb(UI_Dropdown_Struct* e)
{

	int mouth = mouth_to_expend_int[e->Click_Index];
	char str[50];
	UI_Gen_Unlock_Code(str, mouth);
	lv_label_set_text_fmt(Verfiy_Msg.label_UnlockCode, "Unlock code:\n%s\nSelect mouths to expend service", str);
	lv_qrcode_update(Verfiy_Msg.qrcode, str, strlen(str));
}

void UI_Page_Verfiy_Comp(lv_obj_t* parent)
{
	lv_obj_t* verfiyBox = lv_msgbox_create(parent, "Expired", " ", NULL, NULL);
	Verfiy_Msg.Msg_Box = verfiyBox;
	lv_obj_center(verfiyBox);
	lv_obj_set_size(verfiyBox, LV_HOR_RES, LV_VER_RES); // ÈÃËü¸²¸ÇÕû¸öÆÁÄ»
	/*lv_obj_center(list1);*/
	lv_obj_t* label = lv_label_create(verfiyBox);
	lv_label_set_text_fmt(label, " please contact service");
	lv_obj_set_width(label, LV_HOR_RES);
	// lv_obj_align(label, LV_ALIGN_DEFAULT, 100, 0);

	char str[50];
	UI_Gen_Unlock_Code(str, 1);
	lv_obj_t* label_UnlockCode = lv_label_create(verfiyBox);
	Verfiy_Msg.label_UnlockCode = label_UnlockCode;
	lv_label_set_text_fmt(label_UnlockCode, "Unlock code:\n%s\nSelect mouths to expend service", str);
	lv_obj_set_width(label_UnlockCode, LV_HOR_RES - 300);
	Verfiy_Msg.qrcode = lv_qrcode_create(verfiyBox, 80, lv_color_hex(0), lv_color_hex(0xfffff));
	// UI_Dropdown_Struct dropdown_vel = {
	// 	.dropdown = Verfiy_Msg.Mouth.dropdown1,
	// 	.label = Verfiy_Msg.Mouth.label1,
	// 	/*.label_ofs_x = -25,
	// 	.label_ofs_y = 0,
	// 	.x = 50,
	// 	.y = -60,*/
	// 	.Item = mouth_to_expend,
	// 	.parent = Verfiy_Msg.Msg_Box,
	// 	.handle = dropdown_event_cb,
	// 	.Label_Str = ""};
	// Verfiy_Msg.Mouth.obj = dropdown_vel;
	//UI_DropDown_Init(&Verfiy_Msg.Mouth.obj);

	lv_qrcode_update(Verfiy_Msg.qrcode, str, strlen(str));
	// create_custom_keyboard(lv_scr_act(),1);

	/*Create a text area. The keyboard will write here*/
	lv_obj_t* ta;
	ta = lv_textarea_create(verfiyBox);
	Verfiy_Msg.text_Box = ta;
	lv_obj_align(ta, LV_ALIGN_TOP_LEFT, -0, 120);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, 0);
	lv_textarea_set_placeholder_text(ta, "UNLOCK CODE");
	lv_textarea_set_accepted_chars(ta, "1234567890ABCDEF");
	lv_textarea_set_max_length(ta, 6);

	lv_obj_set_size(ta, 240, 45);
	lv_obj_t* submit_btn = lv_btn_create(verfiyBox);
	lv_obj_t* submit_btn_label = lv_label_create(submit_btn);
	Verfiy_Msg.submit_btn = submit_btn;
	Verfiy_Msg.submit_btn_label = submit_btn_label;
	lv_label_set_recolor(submit_btn_label, 1);
	lv_label_set_text_fmt(submit_btn_label, "#000000 Unlock");
	lv_obj_set_style_bg_color(submit_btn, lv_color_hex(0xD3D3D3), 0);
	lv_obj_add_event_cb(submit_btn, Check_Unlock_Code, LV_EVENT_CLICKED, 0);
}

void UI_Page_SetTime(lv_obj_t* parent)
{
	lv_obj_t* verfiyBox;
	verfiyBox = lv_msgbox_create(parent, "TIME SETUP", " ", NULL, true);

	lv_obj_center(verfiyBox);
	lv_obj_set_size(verfiyBox, LV_HOR_RES, LV_VER_RES); // ÈÃËü¸²¸ÇÕû¸öÆÁÄ»

	/*Create a text area. The keyboard will write here*/
	lv_obj_t* ta;
	ta = lv_textarea_create(verfiyBox);
	Verfiy_Msg.time_set_text_Box = ta;
	lv_obj_align(ta, LV_ALIGN_TOP_LEFT, -0, 120);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, Verfiy_Msg.unlock_input_str);
	lv_textarea_set_placeholder_text(ta, "YY,MM,DD,HH,MM");
	lv_textarea_set_accepted_chars(ta, "1234567890,");
	lv_textarea_set_max_length(ta, 20);

	// create_custom_keyboard(lv_scr_act(), 1);

	lv_obj_set_size(ta, 300, 45);
	lv_obj_t* submit_btn = lv_btn_create(verfiyBox);
	lv_obj_t* submit_btn_label = lv_label_create(submit_btn);
	Verfiy_Msg.submit_btn = submit_btn;
	Verfiy_Msg.submit_btn_label = submit_btn_label;
	lv_label_set_recolor(submit_btn_label, 1);
	lv_label_set_text_fmt(submit_btn_label, "#000000 Set time");
	lv_obj_set_style_bg_color(submit_btn, lv_color_hex(0xD3D3D3), 0);
	lv_obj_add_event_cb(submit_btn, Check_Time_Input, LV_EVENT_CLICKED, 0);
	lv_obj_add_event_cb(verfiyBox, TimeBox_Close, LV_EVENT_DELETE, 0);
}

// 0-> timeonly 1->resubscribe 2-> both 3 both allow cancel
void UI_Page_Management_Expored(lv_obj_t* parent, uint8_t _SetupType)
{
	create_custom_keyboard(lv_scr_act(), 1);
	/*if (settimeOnly != 0)
		UI_Page_Verfiy_Comp(parent);*/
	checkbox_toplayer_Parent = parent;
	Setuptag = _SetupType;
	if (Setuptag == timeOnly || Setuptag == Both)
		UI_Page_SetTime(parent);
	if (Setuptag == reSubscribeOnly)
		UI_Page_Verfiy_Comp(parent);
}
