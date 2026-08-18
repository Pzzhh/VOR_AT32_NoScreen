#include "UI_Parameter_Vor.h"
#include "UI_Parameter_Set.h"
#include "../UI_Include.h"
#include "../UI_Component/UI_SpinBox.h"
#include "../UI_Component/UI_Dropdown.h"

const char* Vor_VelItem = "5/s\n""10/s\n""20/s\n""30/s\n" "40/s\n""50/s\n""60/s\n""80/s";
const char* Vor_FreqItem = "0.1HZ\n"
"0.2HZ\n"
"0.3HZ\n"
"0.5HZ\n"
"0.6HZ\n"
"0.8HZ\n"
"1.0HZ\n"
"1.6HZ\n"
"3.2HZ\n"
"5.0HZ\n"
"6.4HZ\n"
"7.2HZ\n"
"8.0HZ\n"
"10HZ";


typedef struct
{
	struct {
		lv_obj_t* label1;
		lv_obj_t* dropdown1;
	}Freq;
	struct {
		lv_obj_t* label1;
		lv_obj_t* dropdown1;
		lv_obj_t* btn1_CW_Set;
		lv_obj_t* btn1_CW_label2;
		uint8_t CW_flag;
	}Vel;
	struct {
		lv_obj_t* label1;
		lv_obj_t* spinbox1;
		int32_t value;
	}counter;
	lv_obj_t* Context_Box;
}UI_parameter_VOR;

static void rotate_dir_buttum_handler(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	uint8_t* flag = e->user_data;
	lv_obj_t* label = lv_obj_get_child(e->current_target, 0);
	if (code == LV_EVENT_CLICKED)
	{
		(*flag) = !(*flag);
	}
	if (1)
	{
		if (*flag)
			lv_label_set_text(label, "CW");
		else
			lv_label_set_text(label, "CCW");
	}
}


static void handler(UI_Dropdown_Struct* e)
{
	LV_LOG_USER("ss %d", e->Click_Index);
}




static UI_Dropdown_Struct dropdown_Vel2, dropdown_vel2;
static UI_parameter_VOR Obj;

static uint8_t UI_Get(Task_Parameter_Struct* e);
static uint8_t UI_Hidden(uint8_t flag);

UI_Parameter_Handler VOR_init(lv_obj_t* parent, int16_t x, int16_t y)
{
	//create base obj
	UI_Parameter_Handler ui_Parameter_Handler = { .hidden = UI_Hidden ,.Get = UI_Get };

	lv_obj_t* context_box = lv_obj_create(parent);
	Obj.Context_Box = context_box;
	lv_obj_clear_flag(context_box, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(context_box, 315, 260);
	lv_obj_set_style_bg_opa(context_box, 0, LV_OPA_TRANSP);
	lv_obj_set_style_border_opa(context_box, 0, LV_OPA_TRANSP);
	lv_obj_align(context_box, LV_ALIGN_DEFAULT, x, y);

	UI_Dropdown_Struct dropdown_freq = {
	   .dropdown = Obj.Freq.dropdown1,
	   .label = Obj.Freq.label1,
	   .label_ofs_x = -14,
	   .label_ofs_y = 0,
	   .x = 50,
	   .y = 0,
	   .Item = Vor_FreqItem,
	   .parent = context_box,
	   .handle = (myhander)handler,
	   .Label_Str = "Freq"
	};
	dropdown_Vel2 = dropdown_freq;
	UI_DropDown_Init(&dropdown_Vel2);
	Obj.Freq.dropdown1 = dropdown_Vel2.dropdown;
	


	UI_Dropdown_Struct dropdown_vel = {
	   .dropdown = Obj.Vel.dropdown1,
	   .label = Obj.Vel.label1,
	   .label_ofs_x = -25,
	   .label_ofs_y = 0,
	   .x = 50,
	   .y = 60,
	   .Item = Vor_VelItem,
	   .parent = context_box,
	   .handle = (myhander)handler,
	   .Label_Str = "Vel"
	};
	dropdown_vel2 = dropdown_vel;
	UI_DropDown_Init(&dropdown_vel2);
	Obj.Vel.dropdown1 = dropdown_vel2.dropdown;
	

	Obj.Vel.btn1_CW_Set = lv_btn_create(context_box);
	lv_obj_add_flag(Obj.Vel.btn1_CW_Set, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_size(Obj.Vel.btn1_CW_Set, 40, 40);
	//label init
	Obj.Vel.btn1_CW_label2 = lv_label_create(Obj.Vel.btn1_CW_Set);
	lv_label_set_text(Obj.Vel.btn1_CW_label2, "CCW");
	lv_obj_align(Obj.Vel.btn1_CW_label2, LV_ALIGN_CENTER, 0, 0);
	lv_obj_align_to(Obj.Vel.btn1_CW_Set, Obj.Vel.dropdown1, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
	//lv_obj_align_to(Obj.Vel.btn1_CW_Set, Obj.Freq.dropdown1, LV_ALIGN_OUT_RIGHT_MID, 15, 0);


	/*Obj.Freq.label1 = lv_label_create(context_box);
	Obj.Freq.dropdown1 = lv_dropdown_create(context_box);

	lv_dropdown_set_options(Obj.Freq.dropdown1, Vor_FreqItem);
	lv_label_set_text(Obj.Freq.label1, "FREQ");

	lv_obj_align(Obj.Freq.dropdown1, LV_ALIGN_LEFT_MID, 50, 0);
	lv_obj_align_to(Obj.Freq.label1, Obj.Freq.dropdown1, LV_ALIGN_OUT_LEFT_MID, -14, 0);
	lv_obj_set_style_text_align(Obj.Freq.label1, LV_TEXT_ALIGN_LEFT, 0);*/

	//Obj.Vel.label1 = lv_label_create(context_box);
	//Obj.Vel.dropdown1 = lv_dropdown_create(context_box);
	//lv_obj_align(Obj.Vel.dropdown1, LV_ALIGN_LEFT_MID, 50, 80);
	//lv_label_set_text_fmt(Obj.Vel.label1, "Vel");
	//lv_obj_align_to(Obj.Vel.label1, Obj.Vel.dropdown1, LV_ALIGN_OUT_LEFT_MID, -25, 0);
	//lv_dropdown_set_options(Obj.Vel.dropdown1, Vor_VelItem);




	//btn is clicked change label text
	lv_obj_add_event_cb(Obj.Vel.btn1_CW_Set, rotate_dir_buttum_handler, LV_EVENT_CLICKED, (void*)&Obj.Vel.CW_flag);
	Obj.counter.value = 100;
	UI_SpinBox_Struct e = {
		 .parent = Obj.Context_Box,
		.SpinBox_Value = &Obj.counter.value,
		.x = 50,
		.y = -60,
		.Label_Str = "Count" };
	SpinBox_Init(&e);
	
	Obj.counter.label1 = e.label;
	Obj.counter.spinbox1 = e.spinbox;


	lv_dropdown_set_selected(Obj.Vel.dropdown1, 5);
	lv_dropdown_set_selected(Obj.Freq.dropdown1, 7);

	return ui_Parameter_Handler;
}

static uint8_t UI_Hidden(uint8_t flag)
{
	static uint8_t Hidden_flag = 0;
	lv_obj_t* parent = (Obj.Context_Box);
	if (lv_obj_is_valid(parent))
	{
		if (flag)
		{
			if (Hidden_flag == 0)
				lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
			Hidden_flag = 1;
		}
		else
		{
			if (Hidden_flag != 0)
				lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
			Hidden_flag = 0;
		}
	}
	return 1;
}

#include "stdio.h"

static uint8_t UI_Get(Task_Parameter_Struct* e)
{
	//mode set
	e->mode = Task_VOR;

	if (Obj.counter.value >= 0)
		e->VOR.Counter = Obj.counter.value;

	char str[10] = { 0 };
	lv_dropdown_get_selected_str(Obj.Freq.dropdown1, str, 10);
	float value = 0;
	sscanf(str, "%f", &value);
	if (value > 11 || value < 0)
		return 0;
	e->VOR.Freq = value;

	char Velstr[10] = { 0 };
	lv_dropdown_get_selected_str(Obj.Vel.dropdown1, Velstr, 10);
	float Vel_value = 0;
	sscanf(Velstr, "%f", &Vel_value);
	if (Vel_value > 360 || Vel_value < -360)
		return 0;

	e->VOR.Vel = Vel_value;

	return 1;
}

//return 1-> ok
uint8_t VOR_exam_Para_Check(Task_Parameter_Struct* e)
{
	if (e->VOR.Counter > 1000 && e->VOR.Counter != 0)
		return 0;
	if (e->VOR.Freq > 11 || e->VOR.Freq < 0)
		return 0;
	if (e->VOR.Vel > 360 || e->VOR.Vel < -360)
		return 0;
	return 1;
}

