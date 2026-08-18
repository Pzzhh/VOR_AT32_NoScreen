#include "UI_Parameter_Set.h"
#include "../Task/task_info_struct.h"
#include "../UI_Include.h"
#include "../UI_Component/UI_SpinBox.h"
#include "../UI_Component/UI_Dropdown.h"

#include "../UI_Parameter/UI_Parameter_Cont.h"
#include "../UI_Parameter/UI_Parameter_Vor.h"
#include "../UI_Parameter/UI_Parameter_Ovar.h"
#include "../UI_Parameter/UI_Parameter_TC.h"
#include "../UI_Parameter/UI_Parameter_VHIT.h"
/*****************************************************/
#define LV_SYMBOL_rotate_right "\xEF\x80\x9E"
/*Add a border with bg_color*/
#define LV_SYMBOL_rotate_left "\xEF\x83\xA2"
LV_FONT_DECLARE(arrow_rotate_right);


#if 0
void Continue_init(lv_obj_t* parent)
{
	static UI_parameter_CONT Obj;
	//create base obj

	lv_obj_t* context_box = lv_obj_create(parent);
	Obj.Context_Box = context_box;
	lv_obj_clear_flag(context_box, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(context_box, 315, 260);
	lv_obj_set_style_bg_opa(context_box, 0, LV_OPA_TRANSP);
	lv_obj_align(context_box, LV_ALIGN_DEFAULT, 100, 0);

	UI_Dropdown_Struct dropdown_freq = {
	   .dropdown = Obj.Vel.dropdown1,
	   .label = Obj.Vel.label1,
	   .label_ofs_x = -14,
	   .label_ofs_y = 0,
	   .x = 50,
	   .y = 0,
	   .Item = Cont_VelItem,
	   .parent = context_box,
	   .handle = handler,
	   .Label_Str = "Vel"
	};
	dropdown_freq2 = dropdown_freq;
	UI_DropDown_Init(&dropdown_freq2);

	Obj.Vel.btn1_CW_Set = lv_btn_create(context_box);
	lv_obj_set_size(Obj.Vel.btn1_CW_Set, 40, 40);
	//label init
	Obj.Vel.btn1_CW_label2 = lv_label_create(Obj.Vel.btn1_CW_Set);
	lv_label_set_text(Obj.Vel.btn1_CW_label2, "CCW");
	lv_obj_align(Obj.Vel.btn1_CW_label2, LV_ALIGN_CENTER, 0, 0);
	lv_obj_align(Obj.Vel.btn1_CW_Set, LV_ALIGN_LEFT_MID, 195, 80);

	//btn is clicked change label text
	lv_obj_add_event_cb(Obj.Vel.btn1_CW_Set, rotate_dir_buttum_handler, LV_EVENT_CLICKED, (void*)&Obj.Vel.CW_flag);
	UI_SpinBox_Struct e = {
		.parent = Obj.Context_Box,
		.label = Obj.counter.label1,
		.spinbox = Obj.counter.spinbox1,
		.SpinBox_Value = &Obj.counter.value,
		.x = 50,
		.y = -60,
		.Label_Str = "Count" };
	SpinBox_Init(&e);
}
#endif
#if 0
#else
const char* Set_item_OKR = "VOR\n""OKR\n""VOR+OKR\n""VOR+OKR*\n""OVAR\n""CONT";
//const char* Set_item_No_OKR = "VOR\n""OVAR\n""CONT";
const char* Set_item_No_OKR = "VOR\n""OVAR";
#endif
typedef struct
{
	struct {
		lv_obj_t* label1;
		lv_obj_t* dropdown1;
		Task_Parameter_Mode_enum Current_Selected;
		char str[10];
	}Mode;
	lv_obj_t* Context_Box;
}UI_parameter_Set_Type;
#define UI_Parameter_Handler_Size 6
typedef struct
{
	UI_Parameter_Handler cb[UI_Parameter_Handler_Size];
}UI_parameter_cb;
UI_parameter_cb ui_mode_cb;
static UI_parameter_Set_Type mode_set;

#define Msg_COMPARE(a,b) (strncmp(a,b,sizeof(a)-1)==0) //前必须是const字符串

static void handler(UI_Dropdown_Struct* e)
{
	// uint8_t flags = 1;
	lv_obj_t* obj = e->dropdown;
	char* str = mode_set.Mode.str;
	lv_dropdown_get_selected_str(obj, str, 10);
	for (int i = 0; i < UI_Parameter_Handler_Size; i++)
	{
		if (ui_mode_cb.cb[i].hidden != 0)
			ui_mode_cb.cb[i].hidden(1);
	}
	if (Msg_COMPARE("VOR", str))
	{
		ui_mode_cb.cb[Task_VOR].hidden(0);
	}
	if (Msg_COMPARE("OKR", str))
	{
		ui_mode_cb.cb[Task_VOR].hidden(0);
	}
	if (Msg_COMPARE("VOR+OKR", str))
	{
		ui_mode_cb.cb[Task_VOR].hidden(0);
	}
	if (Msg_COMPARE("VOR+OKR*", str))
	{
		ui_mode_cb.cb[Task_VOR].hidden(0);
	}
	if (Msg_COMPARE("CONT", str))
	{
		ui_mode_cb.cb[Task_Continue].hidden(0);
	}
	if (Msg_COMPARE("OVAR", str))
	{
		ui_mode_cb.cb[Task_OVAR].hidden(0);
	}
	if (Msg_COMPARE("TC", str))
	{
		ui_mode_cb.cb[Task_TC].hidden(0);
	}
	if (Msg_COMPARE("VHIT", str))
	{
		ui_mode_cb.cb[Task_VHIT].hidden(0);
	}

}


uint8_t UI_Parameter_Read(Task_Parameter_Struct* e)
{
	uint8_t res = 0;
	char* str = mode_set.Mode.str;
	if (Msg_COMPARE("VOR", str))
	{
		res = ui_mode_cb.cb[Task_VOR].Get(e);
		e->VOR.ExMode = Ex_VOR;
	}
	if (Msg_COMPARE("OKR", str))
	{
		res = ui_mode_cb.cb[Task_VOR].Get(e);
		e->VOR.ExMode = Ex_OKR;
	}
	if (Msg_COMPARE("VOR+OKR", str))
	{
		res = ui_mode_cb.cb[Task_VOR].Get(e);
		e->VOR.ExMode = Ex_BOTH;
	}
	if (Msg_COMPARE("VOR+OKR*", str))
	{
		res = ui_mode_cb.cb[Task_VOR].Get(e);
		e->VOR.ExMode = Ex_BOTH_R;
	}
	if (Msg_COMPARE("CONT", str))
	{
		res = ui_mode_cb.cb[Task_Continue].Get(e);
	}
	if (Msg_COMPARE("OVAR", str))
	{
		res = ui_mode_cb.cb[Task_OVAR].Get(e);
	}
	if (Msg_COMPARE("TC", str))
	{
		res = ui_mode_cb.cb[Task_TC].Get(e);
	}
	if (Msg_COMPARE("VHIT", str))
	{
		res = ui_mode_cb.cb[Task_VHIT].Get(e);
	}
	return res;
}
//当作桥梁？
uint8_t UI_Parameter_Set(Task_Parameter_Struct* e)
{
	//Task_Parameter_Struct* e;
	return 0;
}

void Mode_init(lv_obj_t* parent,uint8_t isEnOkR)
{

	static  UI_Dropdown_Struct Mode_dropdown;

	lv_obj_t* context_box = lv_obj_create(parent);
	mode_set.Context_Box = context_box;
	lv_obj_clear_flag(context_box, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(context_box, 220, 50);
	lv_obj_set_style_bg_opa(context_box, 0, LV_OPA_TRANSP);
	lv_obj_set_style_border_opa(context_box, 0, LV_OPA_TRANSP);
	lv_obj_align(context_box, LV_ALIGN_DEFAULT, -15, 55);
	
	UI_Dropdown_Struct dropdown = {
	   .dropdown = mode_set.Mode.dropdown1,
	   .label = mode_set.Mode.label1,
	   .label_ofs_x = -14,
	   .label_ofs_y = 0,
	   .x = 50,
	   .y = 0,
	   .Item = 0,
	   .parent = context_box,
	   .handle = (myhander)handler,
	   .Label_Str = "mode"
	};
	if (isEnOkR)
		dropdown.Item = Set_item_OKR;
	else
		dropdown.Item = Set_item_No_OKR;
	Mode_dropdown = dropdown;
	UI_DropDown_Init(&Mode_dropdown);

	char* str = mode_set.Mode.str;
	lv_dropdown_get_selected_str(Mode_dropdown.dropdown, str, 10); //get str


	ui_mode_cb.cb[Task_VOR] = VOR_init(parent, -15, 130);
	ui_mode_cb.cb[Task_Continue] = Continue_init(parent, -15, 130);
	ui_mode_cb.cb[Task_OVAR] = OVAR_init(parent, -15, 130);
	ui_mode_cb.cb[Task_VHIT] = VHIT_init(parent, -15, 130);
	ui_mode_cb.cb[Task_TC] = TC_init(parent, -15, 130);

	ui_mode_cb.cb[Task_Continue].hidden(1);
	ui_mode_cb.cb[Task_OVAR].hidden(1);
	ui_mode_cb.cb[Task_VHIT].hidden(1);
	ui_mode_cb.cb[Task_TC].hidden(1);
	/*
	 ui_mode_cb.cb[0].hidden(1);*/
	return;
#if 0
	// 闂傚倸鎳忛崝妯何涢敓锟?
	-  /*end*/

		/**< 闂佸搫鍟悥鐓幬涢敓锟? */ time_box = lv_spinbox_create(parent);

	lv_spinbox_set_value(time_box, State.Set_Time);
	lv_spinbox_set_range(time_box, 1, 1000);     // 濡ょ姷鍋涢幊搴ｂ偓瑙勫▕濮婁粙骞囬鈧悡锟?
	lv_spinbox_set_digit_format(time_box, 4, 0); // 闂佽桨绀侀悺銊╁汲閻斿摜鈻旈柕鍫濆缁夛拷
	lv_spinbox_step_prev(time_box);
	lv_obj_set_width(time_box, 100);
	lv_obj_t* time_box_lable = lv_label_create(parent);
	// time_lable = lv_label_create(time_box);
	// lv_obj_align(time_lable, LV_ALIGN_LEFT_MID, -10, 0);
	lv_obj_set_size(time_box, 130, 42);
	lv_obj_align(time_box, LV_ALIGN_LEFT_MID, 110, -40);
	lv_obj_align_to(time_box_lable, time_box, LV_ALIGN_OUT_LEFT_MID, -25, 0);
	lv_obj_clear_flag(time_box, LV_OBJ_FLAG_SCROLLABLE);
	// btn
	lv_obj_t* time_btn_m = lv_btn_create(parent); // 闂佽法鍣﹂幏锟??
	lv_obj_t* time_btn_a = lv_btn_create(parent); // 闂佽法鍣﹂幏锟??
	lv_obj_set_size(time_btn_m, 40, 40);
	lv_obj_set_size(time_btn_a, 40, 40);
	lv_obj_align_to(time_btn_m, time_box, LV_ALIGN_OUT_RIGHT_MID, 65, 0);
	lv_obj_align_to(time_btn_a, time_box, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
	// set
	lv_label_set_text_fmt(time_box_lable, "Time");
	lv_label_set_text_fmt(time_lable, "30s");
	lv_obj_set_style_bg_img_src(time_btn_a, LV_SYMBOL_PLUS, 0);
	lv_obj_set_style_bg_img_src(time_btn_m, LV_SYMBOL_MINUS, 0);
	// btn event
	lv_obj_add_event_cb(time_btn_a, Btn_Num_sever, LV_EVENT_ALL, (void*)1);
	lv_obj_add_event_cb(time_btn_m, Btn_Num_sever, LV_EVENT_ALL, (void*)0);
	/*end*/
	// inc btn      闂佺锕﹂崑鎾诲几閳哄懎绠板鑸靛姈閺嗭拷
	lv_obj_t* inc_btn_label = lv_label_create(parent);
	lv_obj_t* btn_up = lv_btn_create(parent);   // 闂佽法鍣﹂幏锟??
	lv_obj_t* btn_down = lv_btn_create(parent); // 闂佽法鍣﹂幏锟??
	lv_obj_set_size(btn_up, 40, 40);
	lv_obj_set_size(btn_down, 40, 40);
	lv_obj_align(btn_up, LV_ALIGN_BOTTOM_LEFT, 160, -30);
	lv_obj_align_to(inc_btn_label, btn_up, LV_ALIGN_OUT_TOP_MID, 30, -7);
	lv_obj_align_to(btn_down, btn_up, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	lv_label_set_text(inc_btn_label, "INC ");
	lv_obj_set_style_bg_img_src(btn_up, LV_SYMBOL_UP, 0);
	lv_obj_set_style_bg_img_src(btn_down, LV_SYMBOL_DOWN, 0);
	// btn event
	lv_obj_add_event_cb(btn_up, INC_manual_event, LV_EVENT_ALL, (void*)1);
	lv_obj_add_event_cb(btn_down, INC_manual_event, LV_EVENT_ALL, (void*)0);
#endif

}
