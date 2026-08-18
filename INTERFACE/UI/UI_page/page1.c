#include "page1.h"
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

static uint16_t Get_Str_Len(const char* str, uint16_t maxlen);
#define COMPARE(a, b) strncmp(a, b, Get_Str_Len(a, 50))
static Table_Contain_Property* table_Contain_Property;
lv_timer_t Page1_timer;
lv_obj_t* parent_box;
lv_obj_t* parent_bo2[10];
lv_obj_t* start_btn;
lv_obj_t* stop_btn;
lv_obj_t* Msg_Label;
lv_obj_t* MsgWifi_Label;
lv_obj_t* MouseNameTextArea;
lv_obj_t* Task_Btn_Array[4];
char MouseNameTextArea_textSource[50];
uint8_t isSaveUpdata;
uint8_t isPauseAllow = 1;
int16_t CurrentTask = -1, Interval_ID = -1;
int16_t CurrentCount = -1, Interval_RemainingSec = -1;
uint8_t parent_bo2_index = 0;
Task_Parameter_Struct Recive_Task_Info[10];
uint8_t isRemoteRefresh = 0;
uint8_t isTaskBtnClick = 0;
uint8_t isTimeoutMsgShow = 0;
// static char msgBuffer[50];
static int touch_counter = 0;

#define Read_Flag(n) (*((volatile) & n))
typedef enum
{
	Flag_required,
	Flag_OKNE,
	Flag_Error
} Control_flag;
struct
{
	Control_flag flag;
	uint16_t fouce_index;
	Task_Parameter_Struct info;
	Task_Parameter_Struct* info_pt;
} Page_Add_Task;

struct
{
	Control_flag flag;
	uint16_t fouce_index;
	Task_Parameter_Struct Set;
	Task_Parameter_Struct* para;
} Page_Save_Task;

struct
{
	Control_flag flag;
	uint16_t fouce_index;
	Task_Parameter_Struct info;
	Task_Parameter_Struct* info_pt;
} Page_del_Task;
typedef enum
{
	VHIT_BTN_CMD_None,
	VHIT_BTN_CMD_INIT,
	VHIT_BTN_CMD_DEINIT,
	VHIT_BTN_CMD_Enable,
	VHIT_BTN_CMD_Disable
} VHIT_Next_Btn_Enable_CMD_Enum;

struct
{
	enum
	{
		cam_ok,
		cam_l_error,
		cam_r_error,
		cam_b_error,
	} Wifi_Error_Block_typed;
	uint8_t Wifi_Error_Block;
	uint8_t error_begin_at;
	uint8_t LE;
	uint8_t RE;
} UI_Async_Flag;

volatile VHIT_Next_Btn_Enable_CMD_Enum VHIT_Next_Btn_Enable_CMD;

void UI_VHIT_NEXT_BTN_INIT();
void UI_Page1_user_Timeout_Helper();

// void UI_Page1_Refresh_handle(lv_time_t )

static uint16_t Get_Str_Len(const char* str, uint16_t maxlen)
{
	int i = 0;
	for (; i < maxlen; i++)
	{
		if (str[i] == 0)
		{
			return (i == 0) ? 0 : i - 1;
		}
	}
	return 0;
}

void Set_table_Cell_Text(lv_obj_t* obj, Task_Parameter_Struct* e)
{
	// static int8_t updata_count = -1;
	// if (isSaveUpdata == 0)
	//	return;
	static char strs[25];  // 修复：改为静态，避免每次调用分配栈空间
	Table_Property* item_property = UI_Table_Get_Property(obj);
	// if (updata_count > table_Contain_Property->list->size*2 )
	//{
	//	isSaveUpdata = 0;
	//	updata_count = -1;
	// }
	// updata_count++;
	switch (e->mode)
	{
	case Task_VOR:
		if (*item_property->ID == CurrentTask)
			lv_table_set_cell_value_fmt(obj, 0, 1, "%d/%d", CurrentCount, (int)e->VOR.Counter);
		else
			lv_table_set_cell_value_fmt(obj, 0, 1, "%d", (int)e->VOR.Counter);
		sprintf(strs, "%.1f/hz", e->VOR.Freq);
		// sprintf(strs, "%.1f/hz", lv_tick_get() / 1000.0f);
		lv_table_set_cell_value_fmt(obj, 0, 2, "%s", strs);
		sprintf(strs, "%.0f°/s", e->VOR.Vel);
		lv_table_set_cell_value_fmt(obj, 0, 3, "%s", strs);
		break;
	case Task_Continue:
		if (*item_property->ID == CurrentTask)
			lv_table_set_cell_value_fmt(obj, 0, 1, "%ds/%ds", CurrentCount, e->CONT.Sec);
		else
			lv_table_set_cell_value_fmt(obj, 0, 1, "%ds", e->CONT.Sec);
		sprintf(strs, "%.0f°/s", e->CONT.Vel);
		lv_table_set_cell_value_fmt(obj, 0, 2, "%s", strs);
		sprintf(strs, "%.0f°/s", e->CONT.okr_Vel);
		// lv_table_set_cell_value_fmt(obj, 0, 3, "O%s", strs);
		lv_table_set_cell_value_fmt(obj, 0, 3, " ");
		break;
	case Task_OVAR:
		if (*item_property->ID == CurrentTask)
			lv_table_set_cell_value_fmt(obj, 0, 1, "%d/%d", CurrentCount, (int)e->OVAR.Sec);
		else
			lv_table_set_cell_value_fmt(obj, 0, 1, "%ds", (int)e->OVAR.Sec);
		sprintf(strs, "%.1f°", e->OVAR.Inc_Degree);
		lv_table_set_cell_value_fmt(obj, 0, 2, "%s", strs);
		sprintf(strs, "%.0f°/s", e->OVAR.Vel);
		lv_table_set_cell_value_fmt(obj, 0, 3, "%s", strs);
		break;
	case Task_VHIT:
		lv_table_set_cell_value_fmt(obj, 0, 1, "%d", (int)e->VHIT.Counter);
		lv_table_set_cell_value_fmt(obj, 0, 2, "%s", (e->VHIT.IsCW) ? "CW" : "CCW");
		lv_table_set_cell_value_fmt(obj, 0, 3, " ");
		break;
	case Task_TC:
		lv_table_set_cell_value_fmt(obj, 0, 1, "%d", (int)e->TC.Counter);
		sprintf(strs, "%.0f°/s", e->TC.Vel);
		lv_table_set_cell_value_fmt(obj, 0, 2, "%s", strs);
		lv_table_set_cell_value_fmt(obj, 0, 3, " ");
		break;
	}
}
// char *
const char* TaskNameTransLate(Task_Parameter_Struct t)
{
	static const char mode_name_array[][10] = { "VOR", "OKR", "VOR+OKR", "VOR+OKR*" };
	static const char mode_name_array2[][10] = { "VOR", "CONT", "OVAR", "VHIT", "TC" };
	// 注意 这里不是
	uint16_t array_index = t.mode;
	if (t.mode == Task_VOR)
	{
		array_index += t.VOR.ExMode;
		if (array_index >= sizeof(mode_name_array))
			array_index = 0;
		return mode_name_array[(array_index)];
	}
	array_index = t.mode;
	if (array_index >= sizeof(mode_name_array2))
		array_index = 0;
	return mode_name_array2[(array_index)];
}

#define SPRINTF sprintf_s
void ui1_(Table_Property* p)
{
	static int8_t updata_count = -1, currentTask = -1;
	static int16_t current_flag = -1;
	static const char* okstr = LV_SYMBOL_OK;
	static const char* nomral_str = "";
	static const char* running_str = LV_SYMBOL_REFRESH;
	static char str[20];  // 修复：改为静态，避免每次调用分配栈空间
	int flag[2];
	uint8_t readIndex;
	const char* tag = nomral_str;
	Message_Center_Read_prinft("Ctrl", str, sizeof(str), "ReqReadState");
	readIndex = sscanf(str, "ReadState %d %d", flag, flag + 1);
	if (readIndex == 2 && (currentTask != flag[1] || current_flag != flag[0]))
	{
		isSaveUpdata = 1;
		updata_count = -1;
		current_flag = flag[0];
		currentTask = flag[1];
	}
	if (isSaveUpdata == 0)
		return;
	Table_Property* item_property = UI_Table_Get_Property(p->obj);
	if (updata_count > table_Contain_Property->list->size * 2)
	{
		isSaveUpdata = 0;
		updata_count = -1;
	}
	updata_count++;
	short index = *p->ID;
	if (p->Updata_Source)
	{

		Task_Parameter_Struct* task_info = p->Updata_Source;
		if (flag[0] != 0)
		{
			if (index == flag[1])
				tag = running_str;
			else if (index < flag[1])
				tag = okstr;
		}
		lv_table_set_cell_value_fmt(p->obj, 0, 0, "%s%s",
			TaskNameTransLate(*task_info),
			tag);
		Set_table_Cell_Text(p->obj, task_info);
	}
}

void UI_Task_Btn_ADD_Callback(uint16_t fouces_index, Task_Parameter_Struct* pt)
{
	touch_counter++;
	if (pt->mode > Task_TC)
		return;
	lv_obj_t* table = UI_Table_Create(table_Contain_Property, ui1_, fouces_index);
	Table_Property* table_Property = UI_Table_Get_Property(table);
	// parent_bo2[parent_bo2_index++] = table;
	// 设置好数�??�??
	table_Property->Updata_Source = pt;
	UI_Table_Set_Fouces(table_Contain_Property, fouces_index);
}

void UI_Zero_Btn_Click(lv_obj_t* btn, uint8_t isSet)
{
	int zeroEnable = 0;
	// lv_btn_t* btn = e->user_data;
	if (btn == 0)
		return;
	lv_obj_t* label = lv_obj_get_child(btn, 0);
	Message_Center_Read_prinft("Ctrl", &zeroEnable, 1, "DEV Zero Disable");
	if (isSet)
		zeroEnable = !zeroEnable;
	if (!zeroEnable)
	{
		lv_label_set_text(label, "Zero");
	}
	else
	{
		lv_label_set_text(label, "skip");
	}
	Message_Center_Read_prinft("Ctrl", 0, 0, "DEV Zero Disable %d", zeroEnable);
}

void UI_Zero_Btn_Click_Event(lv_event_t* e)
{
	lv_obj_t* btn = e->user_data;
	UI_Zero_Btn_Click(btn, 1);
}

void UI_Task_Btn_ADD_Click_Event(lv_event_t* e)
{
	if (isTaskBtnClick)
		return;
	// isTaskBtnClick = 1;
	uint32_t handleID;
	// volatile Control_flag *add_flag = &Control_Add_Task.flag;		//避免汇编�??访问reg导致错�??
	uint8_t res = UI_Parameter_Read(&Page_Add_Task.info);
	if (res == 0)
	{
		// isTaskBtnClick = 0;
		return;
	}

	int fouces_index = UI_Table_Get_Fouces(table_Contain_Property);
	Page_Add_Task.fouce_index = fouces_index;
	Task_manager_Begin_Req(&handleID);
	res = Task_manager_Req_add(handleID, fouces_index, Page_Add_Task.info);
	Task_manager_End_release(handleID);
	if (res)
		isTaskBtnClick = 0;
	else
		isTaskBtnClick = 1;
}

void UI_Task_Btn_Move_Callback(uint16_t swap_index1, uint16_t swap_index2)
{
	uint16_t fouces_index = 0;
	UI_Table_Move(table_Contain_Property, swap_index1, swap_index2);
	int task_size = UI_Table_Find_Size(table_Contain_Property);
	if (task_size > swap_index2)
	{
		fouces_index = swap_index2;
	}
	UI_Table_Set_Fouces(table_Contain_Property, fouces_index);
}

void UI_Task_Btn_Move_Click_Event(lv_event_t* e)
{
	if (isTaskBtnClick)
		return;
	// isTaskBtnClick = 1;
	uint32_t handleID;
	int fouces_index = UI_Table_Get_Fouces(table_Contain_Property);
	Task_manager_Begin_Req(&handleID);
	uint8_t res = Task_manager_Req_Move(handleID, fouces_index, fouces_index + 1);
	Task_manager_End_release(handleID);
	if (res)
		isTaskBtnClick = 0;
	else
		isTaskBtnClick = 1;
}

void UI_Task_Btn_Save_Callback(uint16_t fouces_index, Task_Parameter_Struct* src, Task_Parameter_Struct* traget)
{
	;
}

void UI_Task_Btn_Save_Click_Event(lv_event_t* e)
{
	if (isTaskBtnClick)
		return;
	// isTaskBtnClick = 1;
	uint32_t handleID = 0;
	Task_Parameter_Struct src;
	uint8_t res = UI_Parameter_Read(&src);
	int fouces_index = UI_Table_Get_Fouces(table_Contain_Property);
	Task_manager_Begin_Req(&handleID);
	uint8_t res2 = Task_manager_Req_Save(handleID, fouces_index, src);
	Task_manager_End_release(handleID);
	if (res)
		isTaskBtnClick = 0;
	else
		isTaskBtnClick = 1;
}

void UI_Task_Btn_Del_Callback(uint16_t fouces_index)
{
	UI_Table_Clean(table_Contain_Property->list, fouces_index, 0);
}

void UI_Task_table_CLEAN_Callback()
{
	int listsize = table_Contain_Property->list->size;
	while (listsize--)
	{
		UI_Table_Clean(table_Contain_Property->list, 0, 0);
	}
}

void UI_Task_Btn_Del_Click_Event(lv_event_t* e)
{
	if (isTaskBtnClick)
		return;
	// isTaskBtnClick = 1;
	static const Task_Parameter_Struct del_templete = { .mode = 0xff };
	int fouces_index = UI_Table_Get_Fouces(table_Contain_Property);
	if (fouces_index < 0)
		return;
	Table_Property* item_property = UI_Table_Find_Obj_User_Data(table_Contain_Property, fouces_index);
	if (item_property == 0)
		return;
	item_property->Updata_Source = (void*)&del_templete;
	LV_LOG_USER("DEL %d", fouces_index);
	uint32_t handleID;
	Task_manager_Begin_Req(&handleID);
	uint8_t res = Task_manager_Req_Del(handleID, fouces_index);
	Task_manager_End_release(handleID);
	if (res)
		isTaskBtnClick = 0;
	else
		isTaskBtnClick = 1;
}

void UI_Zero_Btn_Init(lv_obj_t* parent)
{
	lv_obj_t* btn;
	btn = lv_btn_create(parent);
	lv_obj_align(btn, LV_ALIGN_DEFAULT, 300, 350);
	lv_obj_t* label = lv_label_create(btn); /*Add a label to the button*/
	lv_label_set_text(label, "Zero");		/*Set the labels text*/
	lv_obj_add_event_cb(btn, UI_Zero_Btn_Click_Event, LV_EVENT_CLICKED, btn);
	UI_Zero_Btn_Click(btn, 0);
}

void UI_Task_Btn_Init(lv_obj_t* parent)
{
	static const char string[][5] = { "add", "move", "save", "del" };  // 修复：改为静态常量
	lv_obj_t* btn[4];
	for (int i = 0; i < 4; i++)
	{
		btn[i] = lv_btn_create(parent);
		Task_Btn_Array[i] = btn[i];
		lv_obj_set_size(btn[i], 70, 40);
		lv_obj_t* label = lv_label_create(btn[i]); /*Add a label to the button*/
		lv_label_set_text(label, string[i]);	   /*Set the labels text*/
		lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
		lv_obj_align(btn[i], LV_ALIGN_DEFAULT, i * 80 - 20, 0);
	}
	lv_obj_add_event_cb(btn[0], UI_Task_Btn_ADD_Click_Event, LV_EVENT_CLICKED, 0);
	lv_obj_add_event_cb(btn[1], UI_Task_Btn_Move_Click_Event, LV_EVENT_CLICKED, 0);
	lv_obj_add_event_cb(btn[2], UI_Task_Btn_Save_Click_Event, LV_EVENT_CLICKED, 0);
	lv_obj_add_event_cb(btn[3], UI_Task_Btn_Del_Click_Event, LV_EVENT_CLICKED, 0);
	// parent_bo2 = lv_label_create(parent);
}
char string[50];
void UI_Task_Msg_Init(lv_obj_t* parent)
{
	Msg_Label = lv_label_create(parent);
	lv_obj_align_to(Msg_Label, start_btn, LV_ALIGN_OUT_BOTTOM_LEFT, -350, -30);
	MsgWifi_Label = lv_label_create(parent);
	lv_obj_align_to(MsgWifi_Label, Msg_Label, LV_ALIGN_BOTTOM_MID, 0, -30);
	lv_label_set_text(MsgWifi_Label, "");
	static lv_style_t style;
	lv_style_init(&style);

	// 设置字体（例如使用内置的 Montserrat 字体，大小 24）
	lv_style_set_text_font(&style, &lv_font_montserrat_28);
	lv_obj_set_width(Msg_Label, 350);
	lv_label_set_long_mode(Msg_Label, LV_LABEL_LONG_WRAP);
	// 将样式应用到标签
	lv_obj_add_style(Msg_Label, &style, 0);
	lv_label_set_text(Msg_Label, string);
	lv_label_set_recolor(Msg_Label, 1);
	lv_label_set_recolor(MsgWifi_Label, 1);
}

static uint8_t StartBtnFlag;

void UI_Start_Btn_Clicked_Handle(lv_event_t* e)
{
	static lv_event_code_t flag = 0;
	if (e->code == LV_EVENT_CLICKED)
	{
		if (flag != LV_EVENT_LONG_PRESSED)
		{
			Message_Center_Send_prinft("Ctrl", 0,
				0,
				"ReqShift");
			// printf("\r\n clicked \r\n");
		}
		flag = e->code;
	}
}

void UI_Stop_Btn_Clicked_Handle(lv_event_t* e)
{
	static lv_event_code_t flag = 0;
	if (e->code == LV_EVENT_CLICKED)
	{
		Message_Center_Send_prinft("Ctrl", 0,
			0,
			"ReqStop");
		flag = e->code;
		// printf("\r\n press \r\n");
	}
}

// void UI_Start_Btn_Refresh_Handle(lv_event_t* e)
// {
// 	if (e->code == LV_EVENT_CLICKED)
// 	{
// 		StartBtnFlag = 1;
// 	}
// }

void UI_Start_Btn_Init(lv_obj_t* parent)
{
	lv_obj_t* btn = lv_btn_create(parent);
	start_btn = btn;
	lv_obj_t* label = lv_label_create(btn); /*Add a label to the button*/
	lv_obj_set_size(btn, 70, 50);
	lv_obj_center(label);
	// lv_label_set_text(label, "start");		/*Set the labels text*/
	lv_label_set_recolor(label, 1);
	lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, -15);
	lv_obj_add_event_cb(btn, UI_Start_Btn_Clicked_Handle, LV_EVENT_ALL, 0);

	lv_obj_t* btn_stop = lv_btn_create(parent);
	stop_btn = btn_stop;
	lv_obj_set_size(btn_stop, 100, 50);
	lv_obj_t* stop_label = lv_label_create(btn_stop); /*Add a label to the button*/
	// lv_label_set_text(label, "start");		/*Set the labels text*/
	lv_label_set_recolor(stop_label, 1);
	lv_obj_set_size(btn_stop, 70, 50);
	lv_obj_center(stop_label);
	lv_obj_align_to(btn_stop, btn, LV_ALIGN_LEFT_MID, -100, 0);
	lv_obj_add_event_cb(btn_stop, UI_Stop_Btn_Clicked_Handle, LV_EVENT_ALL, 0);
}

uint8_t UI_Start_Btn_Get_CMD(void)
{
	uint8_t flag = StartBtnFlag;
	StartBtnFlag = 0;
	return flag;
}

char Page1_meassege[50];
uint8_t Page1_meassege_flag;
char* UI_Page1_Get_Msg_Array(uint16_t* len)
{
	if (len != 0)
		*len = sizeof(Page1_meassege);
	if (Page1_meassege_flag == 0)
	{

		Page1_meassege[0] = 0;
		return Page1_meassege;
	}
	else
		return 0;
}

void UI_Page1_Set_Msg_Finish(void)
{
	Page1_meassege_flag = 1;
}

struct
{
	enum
	{
		cmd_none,
		cmd_add,
		cmd_move,
		cmd_del,
		cmd_save,
		cmd_clean,
		cmd_sync
	} cmd_typed;
	Task_Parameter_Struct* task;
	Task_Parameter_Struct* taskArray[15];
	uint32_t task_id;
	uint32_t task_id2;
	uint32_t isvaild;
	uint16_t error_counter;
} task_msg;

void UI_Page1_Send_ADD_Cmd(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	// if (task_msg.isvaild)
	//	return;
	if (Msg_COMPARE("ERROR", msg))
	{
		task_msg.isvaild = 0;
		isTaskBtnClick = 0;
		// return;
	}
	else if (Msg_COMPARE("SYNC", msg))
	{
		int index = 0;
		if (sscanf(msg, "SYNC %d", &index) == 1)
		{
			memcpy(task_msg.taskArray, src, sizeof(Task_Parameter_Struct*) * index);
			task_msg.cmd_typed = cmd_sync;
			task_msg.isvaild = 1;

			task_msg.task_id = index;
		}
		// return;
	}
	else if (Msg_COMPARE("ADD", msg))
	{
		int index = 0;
		if (sscanf(msg, "ADD %d", &index) == 1 &&
			SrcSize == sizeof(void*))
		{
			task_msg.isvaild = 1;
			task_msg.cmd_typed = cmd_add;
			task_msg.task_id = index;
			task_msg.task = (Task_Parameter_Struct*)src;
		}
	}
	else if (Msg_COMPARE("DEL", msg))
	{
		int index = 0;
		if (sscanf(msg, "DEL %d", &index) == 1)
		{
			task_msg.cmd_typed = cmd_del;
			task_msg.task_id = index;
			task_msg.isvaild = 1;
		}
	}
	else if (Msg_COMPARE("Save", msg))
	{
		int index = 0;
		if (sscanf(msg, "Save %d", &index) == 1 &&
			SrcSize == sizeof(Task_Parameter_Struct*))
		{
			task_msg.isvaild = 0;
		}
		isTaskBtnClick = 0;
	}
	else if (Msg_COMPARE("Move", msg))
	{
		int id = 0, id2 = 0;
		if (sscanf(msg, "Move %d %d", &id, &id2) == 2)
		{
			task_msg.cmd_typed = cmd_move;
			task_msg.task_id = id;
			task_msg.task_id2 = id2;
			task_msg.isvaild = 1;
		}
	}
	else if (Msg_COMPARE("SETZERO", msg))
	{
		task_msg.cmd_typed = cmd_clean;
		task_msg.isvaild = 1;
	}
	isSaveUpdata = 1;
}

MsgReadReturn UI_Page1_Send_MouseName(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("MouseName?", msg))
	{
		strncpy(src, MouseNameTextArea_textSource, SrcSize);
		return msg_match;
	}
	return msg_nomatch;
}

MsgReadReturn UI_Page1_Recv_wifi_Block_Msg(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("NoticeWifierror", msg))
	{
		int flag = 0, index = 0, LE = 0, RE = 0;
		if (sscanf(msg, "NoticeWifierror %d %d %d %d", &flag, &index, &LE, &RE) == 4)
		{
			// flag =3 -> all cam ok ->Wifi_Error_Block 0<- all ok
			uint8_t Block = 0;
			switch (flag)
			{
			case 3:
				Block = cam_ok;
				break;
			case 2:
				Block = cam_r_error;
				break;
			case 1:
				Block = cam_l_error;
				break;
			case 0:
				Block = cam_b_error;
				break;
			}
			UI_Async_Flag.Wifi_Error_Block = Block;
			UI_Async_Flag.error_begin_at = index;
			UI_Async_Flag.LE = LE;
			UI_Async_Flag.RE = RE;
		}
		return msg_match;
	}
	return msg_nomatch;
}

MsgReadReturn UI_Page1_Vhit_Next_Handle(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("VHIT_NEXT_INIT", msg))
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_INIT;
		isPauseAllow = 0;
		return msg_match;
	}
	if (Msg_COMPARE("VHIT_NEXT_DEINIT", msg))
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_DEINIT;
		isPauseAllow = 1;
		return msg_match;
	}
	if (Msg_COMPARE("VHIT_NEXT_ENABLE", msg))
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_Enable;
		return msg_match;
	}
	if (Msg_COMPARE("VHIT_NEXT_DISABLE", msg))
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_Disable;
		return msg_match;
	}
	return msg_nomatch;
}

void UI_Page1_Get_Souce_Updata()
{
	if (task_msg.isvaild)
	{
		uint8_t cmdTpyed = task_msg.cmd_typed;
		if (cmdTpyed == cmd_add)
		{
			UI_Task_Btn_ADD_Callback(task_msg.task_id,
				task_msg.task);
		}
		if (cmdTpyed == cmd_move)
		{
			UI_Task_Btn_Move_Callback(task_msg.task_id,
				task_msg.task_id2);
		}
		if (cmdTpyed == cmd_del)
		{
			UI_Task_Btn_Del_Callback(task_msg.task_id);
		}
		if (cmdTpyed == cmd_clean)
		{
			UI_Task_table_CLEAN_Callback();
		}
		if (cmdTpyed == cmd_sync)
		{
			UI_Task_table_CLEAN_Callback();
			for (int i = 0; i < task_msg.task_id; i++)
			{
				UI_Task_Btn_ADD_Callback(i,
					task_msg.taskArray[i]);
			}
			isSaveUpdata = 1;
		}
		task_msg.isvaild = 0;
		isTaskBtnClick = 0;
	}
}


static void msgbox_WifiError_Resume_event_handler(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* msgbox = lv_event_get_current_target(e);

	if (code == LV_EVENT_VALUE_CHANGED) {
		// 获取被点击的按钮索引
		uint16_t btn_id = lv_msgbox_get_active_btn(msgbox);
		lv_msgbox_close(msgbox);
		uint8_t index = UI_Async_Flag.error_begin_at + 1;
		if (btn_id == 0) {
			Message_Center_Send_prinft("Ctrl", &index, sizeof(uint8_t), "ReqStratWithIndex");
		}
		else if (btn_id == 1) {
			index= UI_Async_Flag.error_begin_at;
			Message_Center_Send_prinft("Ctrl", &index, sizeof(uint8_t), "ReqStratWithIndex");
		}
	}
}

volatile uint8_t test_flag = 0;
void UI_Page1_Btn_Refulsh()
{
	enum
	{
		_ready,
		_taskruning,
		_runing, // 没用
		_end,
	} xtask_runing;
	static uint8_t count = 0;
	static State_Bit Last_flag = { .finish = 1 };
	const int mallocSize = 40;
	State_Bit flag;
	Message_Center_Read_prinft("Ctrl", &flag, sizeof(State_Bit), "run_State?");
	// isruning display stop
	// isruning ispause display hold
	// stoping
	// run
	count++;
	if (UI_Async_Flag.Wifi_Error_Block != cam_ok)
	{
		//由于 串口处相反了。简单起见，从这里修改
		static const char* ERROR_MESSAGES[] = {
			"ok",
			"Error 1 Timeout waiting for recording file",
			"Error 2 Wait was cancelled before recording",
			"Error 3 Camera may Record Before Test",
			"Error 4 CAM FTP error",
			"Error 5 Camera did not record",
			"Error 6 WiFi is not connected",
			"Error 7 File stored but low disk space",
			"Error 8 Camera already recording or starting",
			"Error 9 Camera port connect failed",
			"Error 10  Camera port disconnected",
			"Error 11 No response from camera after retries",
			"Error 12 SD card abnormal",
			"Error 13 Unknown Error",
			"Error 14 Start command failed",
			"Error 15 Camera status is not Recording",
			"Error 16 Stop command failed",
			"Error 17 Stop response missing video filename",
			"Error 18 Start did not finish before stop"
		};
		static const char* btns[] = { "Next","Again", "Ignore", NULL};
		// 修复：改为静态数组，避免每次调用都在栈上分配 80 字节
		static const char errormsgT[4][40] = { "none", "check Left camera \nError at Task %d", "check Right camera\nError at Task %d", "check both camera\nError at Task %d" };
		static char errormsgT_Print[70];
		uint8_t startat = (UI_Async_Flag.error_begin_at), LE = UI_Async_Flag.RE, RE = UI_Async_Flag.LE;
		int index = UI_Async_Flag.Wifi_Error_Block;
		index = (index > 4 || index < 0) ? 0 : index;
		index = (index == 1)?2:(index == 2)?1:index; // 由于串口传过来的标志与实际显示的错误信息顺序不一致，这里进行调整
		sprintf(errormsgT_Print, errormsgT[index], startat + 1);
		//const char* erromsg = errormsgT[index];  // 修复：不需要强制转换
		lv_obj_t* mbox1 = lv_msgbox_create(NULL, "error message",
			errormsgT_Print,
			btns, true);
		lv_obj_set_width(mbox1, 500);
		lv_obj_t* label = lv_label_create(mbox1); // 创建一个标签来显示错误信息
		lv_obj_set_width(label, 480);
		lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); // 设置标签文本自动换行
		const uint8_t error_message_count = sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0]);
		const char* left_error_message = (LE < error_message_count) ? ERROR_MESSAGES[LE] : "Unknown Error";
		const char* right_error_message = (RE < error_message_count) ? ERROR_MESSAGES[RE] : "Unknown Error";
		if(index == 1 && LE == 0) left_error_message = "Unknown Error"; // 左摄像头报错但未携带错误码
		if(index == 2 && RE == 0) right_error_message = "Unknown Error"; // 右摄像头报错但未携带错误码
			
		lv_label_set_text_fmt(label, "Left\t:%s\nRight\t:%s", left_error_message, right_error_message); // 设置标签文本为错误信息
		lv_obj_add_event_cb(mbox1, msgbox_WifiError_Resume_event_handler, LV_EVENT_ALL, NULL);
		lv_obj_center(mbox1);
		UI_Async_Flag.Wifi_Error_Block = cam_ok;
	}
	if (Last_flag.flag == flag.flag &&
		Last_flag.ui_para == flag.ui_para)
	{
		return;
	}

	if (flag.IsRunning)
	{
		if (flag.isPause && flag.reqPause == 0)
		{
			// char* color = (count % 16 > 8) ? "#7CFC00" : "#EA0000";
			uint32_t color = (count % 24 > 12) ? 0x7CFC00 : 0xF9F900;
			lv_obj_t* label = lv_obj_get_child(start_btn, 0);
			lv_label_set_text(label, "#000000 " LV_SYMBOL_PLAY);
			test_flag = 1;
			test_flag = 1;
			lv_obj_set_style_bg_color(start_btn, lv_color_hex(color), 0); // green
			test_flag = 2;
			lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEA0000), 0); // gray
		}
		else if (flag.reqPause || flag.ui_para != _taskruning)
		{
			test_flag = 3;
			lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xADADAD), 0); // gray
			test_flag = 4;
			lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xADADAD), 0); // gray
		}
		else if (flag.Exit)
		{
			test_flag = 5;
			lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xADADAD), 0); // gray
			test_flag = 6;
			lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xADADAD), 0); // gray
		}
		else
		{
			lv_obj_clear_flag(stop_btn, LV_OBJ_FLAG_HIDDEN);
			lv_obj_t* label = lv_obj_get_child(stop_btn, 0);
			lv_label_set_text(label, "#000000 " LV_SYMBOL_STOP);
			test_flag = 7;
			lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xEA0000), 0);
			label = lv_obj_get_child(start_btn, 0);
			lv_label_set_text(label, "#000000 " LV_SYMBOL_PAUSE);
			test_flag = 8;
			lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xF9F900), 0); // yellow
		}
	}
	else
	{
		lv_obj_add_flag(stop_btn, LV_OBJ_FLAG_HIDDEN);
		lv_obj_t* label = lv_obj_get_child(start_btn, 0);
		lv_label_set_text(label, "#000000 " LV_SYMBOL_PLAY);
		test_flag = 9;
		lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x7CFC00), 0);
	}
	if (Last_flag.IsRunning != flag.IsRunning)
	{
		if (flag.IsRunning)
		{
			uint8_t i = 0;
			for (i = 0; i < 4; i++)
			{
				lv_obj_clear_flag(Task_Btn_Array[i], LV_OBJ_FLAG_CLICKABLE);
				//lv_obj_add_flag(Task_Btn_Array[i], LV_OBJ_FLAG_HIDDEN);
				test_flag = 10 + i;
				lv_obj_set_style_bg_color(Task_Btn_Array[i], lv_color_hex(0xADADAD), 0);
			}
		}
		else
		{
			uint8_t i = 0;
			for (i = 0; i < 4; i++)
			{
				//lv_obj_clear_flag(Task_Btn_Array[i], LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(Task_Btn_Array[i], LV_OBJ_FLAG_CLICKABLE);
				/*		test_flag = 21 + i;*/
				lv_obj_set_style_bg_color(Task_Btn_Array[i], lv_color_hex(0x2094f0), 0);
			}
		}
	}
	Last_flag = flag;
}

void UI_Page1_Msg_Refulsh()
{
	static char str[40]; // ← 改成静态，只分配一次

	memset(str, 0, sizeof(str));
	if (Message_Center_Read_prinft("Ctrl", str, sizeof(str), "Msg?") == 0)
	{
		lv_label_set_text(Msg_Label, str);
		lv_obj_invalidate(Msg_Label);
	}
	// 无需malloc/free
}

void UI_Page1_Respone_Updata_Msg(uint8_t flag, uint16_t ID)
{
	if (flag == 0 && ID == ID_Bl_ctrl)
	{
	}
}

void UI_DFU_Display()
{

	lv_obj_t* mbox1 = lv_msgbox_create(NULL, "\n		    DFU Fireware Update",
		"",
		0, false);
	lv_obj_center(mbox1);
}
volatile int testcount = 0;
void UI_Page1_Timer_UIupdata_handle(lv_timer_t* t)
{
	static int i = 0;
	static int period = 20; // 50*20ms = 1s
	lv_event_t e;
	/*if (i > period)
	{
		UI_Task_Btn_Del_Click_Event(&e);
		testcount++;
		i = 0;
	}
	else if (i == period / 2)
	{
		UI_Task_Btn_ADD_Click_Event(&e);
	}
	i++;*/
	UI_Page1_Btn_Refulsh();
	UI_Page1_Msg_Refulsh();
	if (isTimeoutMsgShow)
	{
		isTimeoutMsgShow = 0;
		UI_Page1_user_Timeout_Helper();
	}
}

void UI_Page1_Timer_handle(lv_timer_t* t)
{
	UI_Page1_Get_Souce_Updata();
	UI_VHIT_NEXT_BTN_INIT();
}

static void ta_event_cb(lv_event_t* e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* ta = lv_event_get_target(e);
	extern lv_obj_t* UI_get_KeyBorad(lv_obj_t * parent);
	lv_obj_t* kb = UI_get_KeyBorad(lv_scr_act());
	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED)
	{
		lv_keyboard_set_textarea(kb, ta);
		lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
	}
	if (code == LV_EVENT_VALUE_CHANGED)
	{
		strncpy(MouseNameTextArea_textSource,
			lv_textarea_get_text(e->current_target),
			50);
	}

	if (code == LV_EVENT_DEFOCUSED)
	{
		lv_keyboard_set_textarea(kb, NULL);
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		// lv_obj_invalidate(kb);
	}
}

void UI_VHIT_NEXT_BTN_HANDLE(lv_event_t* e)
{
	if (e->code == LV_EVENT_CLICKED)
	{
		Message_Center_Send_prinft("Ctrl", 0, 0,
			"VHIT_Next_CMD");
	}
}

void UI_VHIT_NEXT_BTN_INIT()
{
	static lv_obj_t* VHIT_Next_BTN;
	if (VHIT_Next_Btn_Enable_CMD == VHIT_BTN_CMD_INIT && VHIT_Next_BTN == 0)
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_None;
		VHIT_Next_BTN = lv_btn_create(parent_box);
		lv_obj_set_size(VHIT_Next_BTN, 150, 60);
		lv_obj_align(VHIT_Next_BTN, LV_ALIGN_CENTER, 135, 160 + 20);
		lv_obj_t* label = lv_label_create(VHIT_Next_BTN); /*Add a label to the button*/
		lv_label_set_recolor(label, 1);
		lv_label_set_text(label, "#000000 NEXT VHIT"); /*Set the labels text*/
		lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_style_bg_color(VHIT_Next_BTN, lv_color_hex(0xFFFF00), 0);
		lv_obj_clear_flag(VHIT_Next_BTN, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(VHIT_Next_BTN, UI_VHIT_NEXT_BTN_HANDLE, LV_EVENT_CLICKED, 0);
	}
	else if (VHIT_Next_Btn_Enable_CMD == VHIT_BTN_CMD_DEINIT && VHIT_Next_BTN != 0)
	{
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_None;
		lv_obj_del(VHIT_Next_BTN);
		VHIT_Next_BTN = 0;
	}
	else if (VHIT_Next_Btn_Enable_CMD == VHIT_BTN_CMD_Enable && VHIT_Next_BTN != 0)
	{
		lv_obj_set_style_bg_color(VHIT_Next_BTN, lv_color_hex(0x7CFC00), 0);
		lv_obj_add_flag(VHIT_Next_BTN, LV_OBJ_FLAG_CLICKABLE);
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_None;
	}
	else if (VHIT_Next_Btn_Enable_CMD == VHIT_BTN_CMD_Disable && VHIT_Next_BTN != 0)
	{
		lv_obj_set_style_bg_color(VHIT_Next_BTN, lv_color_hex(0xFFFF00), 0);
		lv_obj_clear_flag(VHIT_Next_BTN, LV_OBJ_FLAG_CLICKABLE);
		VHIT_Next_Btn_Enable_CMD = VHIT_BTN_CMD_None;
	}
}

void UI_mouse_Name_textInput(lv_obj_t* parent)
{
	/*Create a keyboard to use it with an of the text areas*/
	// lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
	// lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

	/*Create a text area. The keyboard will write here*/
	lv_obj_t* ta;
	ta = lv_textarea_create(parent);
	MouseNameTextArea = ta;
	lv_obj_align(ta, LV_ALIGN_TOP_LEFT, -0, 120);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, 0);
	lv_textarea_set_placeholder_text(ta, "enter mouse name");
	lv_textarea_set_accepted_chars(ta, "_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-=+");
	lv_textarea_set_max_length(ta, 10);

	lv_obj_set_size(ta, 240, 45);
}

void UI_Page1_user_Helper()
{
	lv_obj_t* msgbox = lv_msgbox_create(NULL, "Help",
		"1.Please ensure both side cameras are powered on and WiFi is configured.\n"
		"2.Initialization may take up to 2 minutes.\n"
		"3.Manual leveling is required after a power outage.\n"
		"4.Stored tasks can be loaded in settings.\n"
		"5.web access: \n"
		" L: 192.168.31.11:5000 \n"
	"  R: 192.168.31.10:5000 ",
		NULL, true);
	lv_obj_set_width(msgbox, 700);
	lv_obj_center(msgbox);
}

void UI_Page1_user_Timeout_Helper()
{
	lv_obj_t* msgbox = lv_msgbox_create(NULL, "Carmer Connect Timeout",
		"1.Please ensure both side cameras are powered on and WiFi is configured.\n",
		NULL, true);
	lv_obj_set_width(msgbox, 700);
	lv_obj_center(msgbox);
}

void UI_Page1_Send_Helper(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("Init timeout", msg))
	{
		isTimeoutMsgShow = 1;
	}
}

void UI_Helper_Btn_Handler(lv_event_t e)
{
	if (e.code == LV_EVENT_CLICKED)
	{
		UI_Page1_user_Helper();
	}
}

void UI_Helper_Btn_Init(lv_obj_t* parent)
{
	lv_obj_t* btn;
	btn = lv_btn_create(parent);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 10);
	lv_obj_t* label = lv_label_create(btn); /*Add a label to the button*/
	lv_label_set_recolor(label, 1);
	lv_label_set_text(label, "#000000 Help");				   /*Set the labels text*/
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff), 0); // green
	lv_obj_add_event_cb(btn, UI_Helper_Btn_Handler, LV_EVENT_CLICKED, 0);
}

void Page1_init(lv_obj_t* parent)
{
	parent_box = parent;
	table_Contain_Property = UI_ListBox_Create(parent);
	uint32_t isEnOkr = 0;
	Message_Center_Read_prinft("Ctrl", &isEnOkr, sizeof(isEnOkr), "DEV C610 CONFIG");

	// user expirement para set
	Mode_init(parent, isEnOkr);
	// UI_Table_Create(list5, 0);
	/*btn*/
	UI_Task_Btn_Init(parent); // task btn include -add-save-move-del

	UI_Helper_Btn_Init(parent);
	// UI_Zero_Btn_Init(parent);
	//  task begin btn
	UI_Start_Btn_Init(parent);
	UI_Task_Msg_Init(parent);

	UI_Page1_Outage_Notice();
	// meassage
	Message_Center_Add_Send_CB("task", UI_Page1_Send_ADD_Cmd);
	Message_Center_Add_Read_CB("page1", UI_Page1_Send_MouseName);
	Message_Center_Add_Send_CB("page1", UI_Page1_Vhit_Next_Handle);
	Message_Center_Add_Send_CB("page1", UI_Page1_Recv_wifi_Block_Msg);
	Message_Center_Add_Send_CB("page1", UI_Page1_Send_Helper);
	// mouse ID inter
	UI_mouse_Name_textInput(parent);

	lv_timer_create(UI_Page1_Timer_handle, 5, 0);
	lv_timer_create(UI_Page1_Timer_UIupdata_handle, 100, 0);
}
