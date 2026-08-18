#include "control.h"
#include "control_Hardware_API.h"
#include "../../commuication/communication_API.h"
#include "task_control.h"
#include "../other/system_function.h"
#include "stdint.h"
#include "../other/meassage_center.h"
#include "../Task/task_manager.h"
#include "stdarg.h"
#include "../varify/verfiy.h"

#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#endif // use_windows

#define Using_UI_LOCK 1

#define use_windows
#ifdef ATM32F43_xx
#else
typedef uint32_t TaskHandle_t;
#endif
// void thread_create(void* function, Task_control_info* e, TaskHandle_t* control_thread, uint16_t sizeofstack);
TaskHandle_t task_ctrl_thread, contrl_thread;
Task_control_info control_info = { 0 };
uint8_t inc_dir_g;
uint8_t Uart_Test_Pass = 0, Uart_Test_Req = 0;

#if 0
UI_Function_struct control_cb_array[control_cb_array_size];
uint8_t Control_Set_cb(UI_Function_struct* function_array,
	uint16_t index,
	UI_function cb)
{
	for (int i = 0; i < index; i++)
	{
		if (function_array[i].cb == 0)
		{
			function_array[i].cb = cb;
			return 1;
		}
	}
	return 0;
}

uint8_t Control_Remove_cb(UI_Function_struct* function_array,
	uint16_t index,
	UI_function cb)
{
	for (int i = 0; i < index; i++)
	{
		if (function_array[i].cb == cb)
		{
			function_array[i].cb = 0;
			return 1;
		}
	}
	return 0;
}

void control_run_cb_array(UI_Function_struct* function_array,
	uint16_t index,
	char cmd[50],
	char message[50],
	Task_Parameter_Struct* e)
{
	for (int i = 0; i < index; i++)
	{
		if (function_array[i].cb != 0)
		{
			uint8_t res = function_array[i].cb(message, cmd, 0);
			/*if (function_array[i].cb(message, cmd, e))*/
			/*return;*/
		}
	}
}
#endif

typedef enum
{
	StartCmdNone,
	StartCmdBegin,
	StartCmdStop,
	StartCmdPause,
	StartCmdBeginLongPress
} Start_Cmd_Type;
Start_Cmd_Type Start_Stop_Flag = StartCmdNone;
uint8_t StartATindex = 0;

uint8_t Ctrl_Get_Strat_Cmd()
{
	uint8_t cmd = Start_Stop_Flag;
	if (Start_Stop_Flag == StartCmdBegin)
	{
		control_info.StartWithIndex = StartATindex;
		StartATindex = 0;
	}
	if (Start_Stop_Flag != StartCmdNone)
		Start_Stop_Flag = StartCmdNone;
	return cmd;
}

uint8_t Ctrl_Msg_Printf(const char* format,
	...)
{
	va_list args;
	va_start(args, format);
	uint16_t stringLen = vsprintf(control_info.message, format, args);
	control_info.ismessageUpdata = 1;
	va_end(args);
}

uint8_t Ctrl_WifiMsg_Printf(const char* format,
	...)
{
	va_list args;
	va_start(args, format);
	uint16_t stringLen = vsprintf(control_info.Wifimessage, format, args);
	control_info.isWifimessageUpdata = 1;
	va_end(args);
}

uint8_t Ctrl_Save_Config()
{
	HAL_CONFIG_WRITE(0,
		(uint8_t*)&control_info.Dev_Mode_Bit.flag,
		sizeof(control_info.Dev_Mode_Bit.flag));
}

uint8_t Ctrl_Resume_Config()
{
	uint8_t buff[10];
	int res = HAL_CONFIG_READ(0, buff, 8);
	if (res != 0)
	{
		memcpy(&control_info.Dev_Mode_Bit.flag,
			buff,
			sizeof(control_info.Dev_Mode_Bit.flag));
	}
	if (control_info.Dev_Mode_Bit.UARTMODE == 2)
		control_info.Rk3588_Flag.isUsingBlock = 1;
	else
		control_info.Rk3588_Flag.isUsingBlock = 0;
	// control_info.Dev_Mode_Bit.C610 = 1;
	// // control_info.Dev_Mode_Bit.Inc_DIR = 0;
	// inc_dir_g = 1;
}

char* GetRunState()
{
	static const char st_msg[][10] = { "init", "ready", "pre", "run", "terminal", "pause" };
	if (control_info.State_Bit.H_init)
	{
		return "init"; // init
	}
	if (control_info.State_Bit.IsRunning)
	{
		switch (control_info.UI_para.state)
		{
		case taskInv:
			return "inv";
		case taskinit:
			return "pre";
		case taskruning:
			return "run";
		case taskTerminal:
			return "terminal";
		case taskPause:
			return "pause";
		case taskend:
			return "finish";
		default:
			break;
		}
	}
	else
	{
		return "ready";
	}
}

// return 0 ->ok
uint8_t Ctrl_Read_Ack(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("ReqReadState", msg))
	{
		sprintf(src, "ReadState %d %d",
			control_info.State_Bit.IsRunning, control_info.currentCount);
		return 0;
	}
	if (Msg_COMPARE("GetRunInfo", msg))
	{
		sprintf(src, "RunInfo  %s %d",
			GetRunState(), control_info.currentCount + 1);
		return 0;
	}
	if (Msg_COMPARE("ReqTaskMsg", msg))
	{
		sprintf(src, "TaskMsg %s",
			control_info.message);
		return 0;
	}
	if (Msg_COMPARE("ISCtrlInit", msg))
	{
		sprintf(src, "ISCtrlInit %d",
			control_info.State2_Bit.H_init);
		return 0;
	}
	if (Msg_COMPARE("IsInitFinish", msg))
	{
		if (src != 0 && SrcSize == sizeof(uint8_t))
		{
			uint8_t* flag = (uint8_t*)src;
			flag[0] = control_info.State_Bit.powerUp;
		}
		return 0;
	}
	if (Msg_COMPARE("run_State?", msg))
	{
		if (SrcSize == sizeof(uint32_t))
		{
			uint16_t* flag = (uint16_t*)src;
			flag[0] = control_info.State_Bit.flag;
			flag[1] = control_info.UI_para.state;
		}
		return 0;
	}
	if (Msg_COMPARE("H_run_State", msg))
	{

		sprintf(src, "H_run_State %d",
			control_info.State_Bit);
		return 0;
	}
	if (Msg_COMPARE("Msg?", msg))
	{
		if (control_info.ismessageUpdata)
		{
			strncpy(src, control_info.message, SrcSize);
			control_info.ismessageUpdata = 0;
			return 0;
		}
	}
	if (Msg_COMPARE("WifiMsg?", msg))
	{
		if (control_info.isWifimessageUpdata)
		{
			strncpy(src, control_info.Wifimessage, SrcSize);
			control_info.isWifimessageUpdata = 0;
			return 0;
		}
	}
	if (Msg_COMPARE("isNeedSetup", msg))
	{
		if (SrcSize == sizeof(int))
		{
			int* value = (int*)src;
			if (value != 0)
			{
				control_info.isExpired = Verify_Check_isExpired();
				*value = control_info.isExpired;
			}
		}
	}
	if (Msg_COMPARE("GetLicense", msg))
	{
		if (SrcSize == sizeof(timetyped))
		{
			timetyped* t = (timetyped*)src;
			uint8_t res = Verify_get_license_Date(t);
		}
	}
	if (Msg_COMPARE("ReqTime", msg))
	{
		if (SrcSize == sizeof(timetyped) &&
			src != 0)
		{
			HAL_TIME_Get((timetyped*)src);
		}
	}

	return 1;
}

uint8_t Ctrl_Write_Ack(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("ReqShift", msg))
	{
		if (control_info.State_Bit.IsRunning)
			Start_Stop_Flag = StartCmdPause;
		else
			Start_Stop_Flag = StartCmdBegin;
	}
	if (Msg_COMPARE("ReqStrat", msg))
	{
		Start_Stop_Flag = StartCmdBegin;
	}
	if (Msg_COMPARE("ReqStop", msg))
	{
		Start_Stop_Flag = StartCmdStop;
	}
	if (Msg_COMPARE("ReqStratWithIndex", msg))
	{
		if (src != 0 && SrcSize == sizeof(uint8_t))
		{
			StartATindex = *(uint8_t*)src;
			Start_Stop_Flag = StartCmdBegin;
		}
	}
	if (Msg_COMPARE("SetTime", msg))
	{

		if (SrcSize == sizeof(timetyped) &&
			src != 0)
		{
			timetyped* t;
			t = (timetyped*)src;
			Verify_Set_Time((void*)t);
		}
	}
	if (Msg_COMPARE("SetVaildTime", msg))
	{

		if (SrcSize == sizeof(uint8_t) &&
			src != 0)
		{
			uint8_t* month = src;
			Verify_Set_Subscribe(*month);
		}
	}
	return 0;
}

uint8_t Maintain_Service_Read_ack(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("Camere LED", msg))
	{
		uint8_t res = HAL_CAM_SET_sign_led();
		return !res;
	}
	if (Msg_COMPARE("Camere wifi", msg))
	{
		uint8_t res = HAL_CAM_SET_Set();
		return !res;
	}
	if (Msg_COMPARE("Camere Rec", msg))
	{
		uint8_t res = HAL_CAM_REC_Set(2);
		return !res;
	}
	if (Msg_COMPARE("Camere Vol", msg))
	{
		int volX1000 = 0;
		uint8_t res = 1; // 0->success
		if (sscanf(msg, "Camere Vol %d", &volX1000) == 1)
		{
			if (volX1000 < 1800 && volX1000 > 500)
				res = HAL_CAM_SET_Led_Voltage(volX1000);
			// if (res == 0)
			sprintf(src, "fail %d", res);
		}
		else
			sprintf(src, "f %d", res);
		return !res;
	}
	if (Msg_COMPARE("Inc Up", msg))
	{
		HAL_Incline_Fouces_Move(1);
	}
	if (Msg_COMPARE("Inc down", msg))
	{
		HAL_Incline_Fouces_Move(-1);
	}
	if (Msg_COMPARE("task load", msg))
	{
		int index = 0;
		int res = 0;
		if (sscanf(msg, "task load %d", &index) == 1)
		{
			if (index < 4)
			{
				//这里有问题  
				static Task_Parameter_Struct TaskArray[10];
				int r = sizeof(Task_Parameter_Struct);
#ifdef ATM32F43_xx
				taskENTER_CRITICAL();
#endif
				res = HAL_TaskStroage_Read(index, TaskArray);
#ifdef ATM32F43_xx
				taskEXIT_CRITICAL();
#endif
				if (res > 0)
					Task_manager_Req_Reload(0, res, TaskArray);
			}
		}
		if (SrcSize == sizeof(uint8_t))
			*src = (res) ? 1 : 0;
	}
	if (Msg_COMPARE("Outage Resume", msg))
	{
		HAL_Outage_Resume();
	}
	if (Msg_COMPARE("Outage Clean", msg))
	{
		HAL_Outage_Log(0, 0);
	}
	if (Msg_COMPARE("isOutage?", msg))
	{
		uint8_t value[2] = { 0 };
		int res = HAL_Outage_Read(value, sizeof(value));
		if (res)
		{
			if (src != 0 && SrcSize == sizeof(uint8_t))
				*src = value[0];
		}
	}
	if (Msg_COMPARE("ReadOutageIndex", msg))
	{
		uint8_t value[2] = { 0 };
		int res = HAL_Outage_Read(value, sizeof(value));
		if (res)
		{
			if (src != 0 && SrcSize == sizeof(uint8_t))
				*src = value[1];
		}
	}
	if (Msg_COMPARE("OutageTime?", msg))
	{
		if (src != 0 && SrcSize == sizeof(timetyped))
		{
			Hal_Outage_time_Read((timetyped*)src);
		}
	}
	if (Msg_COMPARE("task save", msg))
	{
		int index = 0;
		int res = 0;
		if (sscanf(msg, "task save %d", &index) == 1)
		{
			if (index < 4)
			{
				Task_Parameter_Struct TaskArray[10];
				int res = HAL_Task_GetArray(TaskArray, 10);
#ifdef ATM32F43_xx
				taskENTER_CRITICAL();
#endif
				if (res != 0)
					HAL_TaskStroage_Write(index, TaskArray, res);
#ifdef ATM32F43_xx
				taskEXIT_CRITICAL();
#endif
			}
		}
		if (SrcSize == sizeof(uint8_t) && src != 0)
			*src = (res) ? 1 : 0;
	}
	if (Msg_COMPARE("FAN_pwm", msg))
	{
		int pwm = 0;
		if (sscanf(msg, "FAN_pwm %d", &pwm) == 1)
		{
			HAL_FAN_Set(pwm);
		}
		return 0;
	}
	if (Msg_COMPARE("DEV LOOP", msg))
	{
		int value;
		if (sscanf(msg, "DEV LOOP %d", &value) == 1)
		{
			if (value >= 0 && value <= 1)
				control_info.Dev_Mode_Bit.LoopTest = value;
		}
		int* Returnvalue = (int*)src;
		*Returnvalue = control_info.Dev_Mode_Bit.LoopTest;
		return 0;
	}
	if (Msg_COMPARE("DEV UART CONFIG", msg))
	{
		int value;
		if (sscanf(msg, "DEV UART CONFIG %d", &value) == 1)
		{
			if (value >= 0 && value <= 2)
				control_info.Dev_Mode_Bit.UARTMODE = value;
			if (value == 2)
				control_info.Rk3588_Flag.isUsingBlock = 1;
			else
			{
				control_info.Rk3588_Flag.isUsingBlock = 0;
			}
		}
		if (src != 0)
		{
			int* Returnvalue = (int*)src;
			*Returnvalue = control_info.Dev_Mode_Bit.UARTMODE;
		}
		return 0;
	}
	if (Msg_COMPARE("DEV TIME ", msg))
	{
		int value[6] = { 0 };
		int res = sscanf(msg, "DEV TIME %d/%d/%d/%d/%d/%d", &value[0], &value[1], &value[2],
			&value[3], &value[4], &value[5]);
		if (res == 6)
		{
		}
	}
	if (Msg_COMPARE("FACTORY Reset", msg))
	{
#ifdef ATM32F43_xx
		extern uint8_t Verify_Factory_Reset(void);
		extern void RequestReset(void);
		Verify_Factory_Reset();
		RequestReset();
#endif
	}
	if (Msg_COMPARE("UART Test", msg))
	{
		Uart_Test_Req = 1;
		ControlDelay(100);
		uint32_t* num = (uint32_t*)src;
		if (num != 0 && SrcSize == sizeof(uint32_t))
		{
			*num = Uart_Test_Pass;
		}
	}
	if (Msg_COMPARE("ENCODE Test", msg))
	{
	}
	if (Msg_COMPARE("DFU", msg))
	{
#ifdef ATM32F43_xx
		extern void RequestEnterDFU(void);
		RequestEnterDFU();
#endif
	}
	if (Msg_COMPARE("CONFIG SAVE", msg))
	{
		Ctrl_Save_Config();
	}
	if (Msg_COMPARE("GET C610 Factor", msg))
	{
#ifdef ATM32F43_xx
		extern int C610SpdCompensetion;
		uint64_t* num = (uint64_t*)src;
		*num = (uint64_t)&C610SpdCompensetion;
#else
		static int C610SpdCompensetion = 500;
		uint64_t* num = (uint64_t*)src;
		*num = (uint64_t)&C610SpdCompensetion;

#endif
	}
	if (Msg_COMPARE("DEV C610 CONFIG", msg))
	{
		int ison = 0;
		uint32_t* num = (uint32_t*)src;

		int res = sscanf(msg, "DEV C610 CONFIG %d", &ison);
		if (res == 1)
		{
			if (ison)
				control_info.Dev_Mode_Bit.C610 = 1;
			else
				control_info.Dev_Mode_Bit.C610 = 0;
		}
		if (num != 0)
			*num = control_info.Dev_Mode_Bit.C610;
	}
	if (Msg_COMPARE("DEV Zero Disable", msg))
	{
		int ison = 0;
		uint32_t* num = (uint32_t*)src;

		int res = sscanf(msg, "DEV Zero Disable %d", &ison);
		if (res == 1)
		{
			if (ison)
				control_info.Dev_Mode_Bit.Zero = 1;
			else
				control_info.Dev_Mode_Bit.Zero = 0;
		}
		if (num != 0)
			*num = control_info.Dev_Mode_Bit.Zero;
	}
	return 1;
}

uint8_t ctrlWaitRk3588()
{
	uint8_t Startflag = Ctrl_Get_Strat_Cmd();
	if (Startflag != 0)
	{
		return 1;
	}
	return 0;
}

uint8_t Rk3588_Ack_Cmd_Handle(Task_control_info* e, uint8_t LR)
{
	const uint16_t Rk_Ack_Expiration = 200; // 2000ms
	static uint16_t Rk_Ack_CountL, Rk_Ack_CountR;
	static uint8_t last_Rk3588_FlagL, last_Rk3588_FlagR;
	uint16_t* Rk_Ack_Count = (LR) ? &Rk_Ack_CountL : &Rk_Ack_CountR;
	uint8_t* last_Rk3588_Flag = (LR) ? &last_Rk3588_FlagL : &last_Rk3588_FlagR;
	uint8_t* flag = (LR) ? &e->Rk3588_Flag.Lflag : &e->Rk3588_Flag.Rflag;
	uint8_t* ErrorCode = (LR) ? &e->Rk3588_Flag.LerrorCode : &e->Rk3588_Flag.RerrorCode;
	static char message[50];
	static char wifiNamestr[2][20];
	static char ProcessInfoStr[2][20];
	uint8_t res = (LR) ? Hal_Rk3588_R_Readarray(message) : Hal_Rk3588_L_Readarray(message);
	if (res == 1)
	{
		if (Msg_COMPARE("status:", message))
		{
			int error_value = 0;
			//memset(getstatus, 0, sizeof(getstatus));	
			if (sscanf(message, "status:%d", &error_value) == 1)
			{
				HAL_USB_Log_Printf("[%d]Rk3588 %s status:%d %s\n", ControlGetTick(), (LR) ? "R" : "L", error_value,message);
				if (error_value == 0)
				{
					e->Rk3588_Flag.WifiMsgFlag |= (LR) ? 2 : 1;
				}
				*ErrorCode = (uint8_t)error_value;
			}
			*flag = Rk3588_Uart_Idle;
		}
		if (Msg_COMPARE("ok", message))
		{
			//	if (*flag == SendTaskArray || *flag == Pause) // when send task array
			*flag = Rk3588_Uart_Idle;
		}

		if (Msg_COMPARE("ready", message))
		{
			if (e->Dev_Mode_Bit.UARTMODE == 2)
				e->State_Bit.powerUp &= ~((LR) ? 2 : 1); // left 0:->01'2b right 1:->10'2b
			else
				e->State_Bit.powerUp = 0;
		}
		if (Msg_COMPARE("DFU", message))
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
				"DFU");
		}
		if (Msg_COMPARE("FACTORY Reset", message))
		{
			Message_Center_Read_prinft("Ctrl", 0, 0,
				"FACTORY Reset");
		}
		if (Msg_COMPARE("UART Test", message))
		{
			Uart_Test_Pass = (LR) ? (Uart_Test_Pass | 2) : (Uart_Test_Pass | 1);
		}
		if (Msg_COMPARE("reqtime", message))
		{
			// Remote requests current datetime — respond with datetime:YYYY-MM-DDTHH:MM:SS\n
			extern uint8_t HAL_TIME_Get(timetyped *time);
			timetyped t = { 0 };
			HAL_TIME_Get(&t);
			if (LR)
				Rk3588_R_Printf("datetime:%04d-%02d-%02dT%02d:%02d:%02d\n",
					t.date.year, t.date.month, t.date.date,
					t.date.hour, t.date.min, t.date.sec);
			else
				Rk3588_L_Printf("datetime:%04d-%02d-%02dT%02d:%02d:%02d\n",
					t.date.year, t.date.month, t.date.date,
					t.date.hour, t.date.min, t.date.sec);
			/*HAL_USB_Log_Printf("[%d]datetime sync -> %04d-%02d-%02dT%02d:%02d:%02d\n",
				ControlGetTick(),
				t.date.year, t.date.month, t.date.date,
				t.date.hour, t.date.min, t.date.sec);*/
		}
		if (Msg_COMPARE("wifiname:", message))
		{

			char* wifistr = (LR) ? wifiNamestr[1] : wifiNamestr[0];
			sscanf(message, "wifiname:%s", wifistr);
			Ctrl_WifiMsg_Printf("L:%s-R:%s", wifiNamestr[0], wifiNamestr[1]);
			e->Rk3588_Flag.WifiMsgFlag |= (LR) ? 2 : 1;
			//e->Rk3588_Flag.WifiMsgFlag = 3;
		}
	}
	if (*last_Rk3588_Flag != *flag)
	{
		*Rk_Ack_Count = 0;
	}

	if (*flag == ReqRKErrorStatus)
	{
		RK3588_ReqRkErrorStatus_Printf(LR);
		*ErrorCode = 8;
		*flag = Rk3588_Uart_Idle;
	}

	if (*flag == SendTaskArray)
	{
		if (e->State_Bit.IsRunning == 0)
			*flag = 0;
		if (*Rk_Ack_Count == 0)
		{
			char* wifistr = (LR) ? wifiNamestr[1] : wifiNamestr[0];
			wifistr[0] = 0;
			Ctrl_WifiMsg_Printf("L:%s-R:%s", wifiNamestr[0], wifiNamestr[1]);
			extern void RK3588_SendTaskInfo(uint16_t tasksize, uint8_t LR);
			RK3588_SendTaskInfo(e->currentCount, LR);
			*Rk_Ack_Count = Rk_Ack_Expiration;
		}
		else
		{
			*Rk_Ack_Count = *Rk_Ack_Count - 1;
		}
		*flag = Rk3588_Uart_Idle;
	}
	if (*flag == RecordStop)
	{
		RK3588_RecordStop_Printf(LR);
		*flag = Rk3588_Uart_Idle;
	}
	if (*flag == Cancel)
	{
		*flag = Rk3588_Uart_Idle;
		// if (*Rk_Ack_Count == 0)
		//{
		//	e->State_Bit.WaitRk = 1;
		//	extern void RK3588_terminal_Printf(uint8_t LR);
		//	RK3588_terminal_Printf(LR);
		//	// Ctrl_Msg_Printf("Cleaning Video");
		//	*Rk_Ack_Count = Rk_Ack_Expiration;
		// }
		// else
		//{
		//	*Rk_Ack_Count = *Rk_Ack_Count - 1;
		// }
	}
	if (*flag == Pause)
	{
		if (*Rk_Ack_Count == 0)
		{
			extern void RK3588_pause_Printf(uint8_t LR);
			RK3588_pause_Printf(LR);
			*Rk_Ack_Count = Rk_Ack_Expiration;
		}
		else
		{
			*Rk_Ack_Count = *Rk_Ack_Count - 1;
		}
		*flag = Rk3588_Uart_Idle;
	}
	if (Uart_Test_Req)
	{
		extern void RK3588_SendTestCmd();
		RK3588_SendTestCmd();
		Uart_Test_Req = 0;
		Uart_Test_Pass = 0;
	}

	*last_Rk3588_Flag = *flag;
}

void Ctrl_info_Indicator(Task_control_info* e)
{
	const int16_t timeout = 120;
	static uint32_t count = 0;
	uint32_t tickSec = ControlGetTick() / 1000;
	// static uint8_t IntCount;
	static int16_t countDown = 1;
	const char dot[3][4] = { ".", "..", "..." };
	if (count == 0)
		count = tickSec + 1;
	if (countDown >= 0)
		countDown = timeout - (tickSec - count);
	else
		countDown = -1;
	if (e->State_Bit.flag == 0)
		Ctrl_Msg_Printf("#228B22 Ready#");
	if (e->State_Bit.powerUp)
	{
		char* msg = (countDown != -1) ? "#000000 Initializing# %03d" : (tickSec % 2 == 0) ? "#EA0000 Init timeout#"
			: "Init timeout";
		Ctrl_Msg_Printf(msg, countDown);
		if (countDown == 0)
		{
			countDown = -1;
			Message_Center_Send_prinft("page1", 0, 0,
				"Init timeout");
		}
	}
	if (e->State_Bit.WaitRk)
	{
		Ctrl_Msg_Printf("proceeding video");
		// Ctrl_Msg_Printf("proceeding video %d %d", e->Rk3588_Flag.Lflag, e->Rk3588_Flag.Rflag);
	}
	// IntCount = (IntCount >= 2) ? 0 : IntCount + 1;
}
void Ctrl_Vhit_Ack(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("VHIT_Next_CMD", msg))
	{
		control_info.State_Bit.VHIT_Next = 1;
	}
}

void Maintain_Service()
{
}

uint8_t Ctrl_Message_Center_init()
{
	/*Meassage_Center_Add("Ctrl");*/
	Message_Center_Add_Read_CB("Ctrl", Ctrl_Read_Ack);
	Message_Center_Add_Send_CB("Ctrl", Ctrl_Write_Ack);
	Message_Center_Add_Read_CB("Ctrl", Maintain_Service_Read_ack);
	Message_Center_Add_Send_CB("Ctrl", Ctrl_Vhit_Ack);
}

void controlfunction()
{
	extern void Task_mangager_Init();
	extern void Communication_Init();
	static Task_Parameter_Struct taskarray[15];
	int flag = 0;
	Start_Cmd_Type Startflag = StartCmdNone;
	uint8_t nowState = 0;

	thread_create(Task_control_handler, &control_info, &task_ctrl_thread, 1000, "task_control", 9);
	/*Meassage_Center_Add("page1");*/
	Ctrl_Message_Center_init();
	HAL_API_INIT();
	if (HAL_Get_C610_Status())
		control_info.Dev_Mode_Bit.C610 = 0;
	Task_mangager_Init();
	Communication_Init();
	Ctrl_Resume_Config();
	// control_info.isExpired = Verify_Check_isExpired();
	control_info.State2_Bit.H_init = 1; // for ui sync
	control_info.State_Bit.powerUp = 3;

	// DateTime sync: send reqtime to remote on startup (protocol: remote replies datetime:YYYY-MM-DDTHH:MM:SS\n)
	/*extern void Rk3588_R_Printf(const char* strOutputString, ...);
	extern void Rk3588_L_Printf(const char* strOutputString, ...);
	Rk3588_R_Printf("reqtime\n");
	Rk3588_L_Printf("reqtime\n");*/
	while (1)
	{

		Ctrl_info_Indicator(&control_info);
		if (control_info.State_Bit.isRun == 0)
		{
			Startflag = Ctrl_Get_Strat_Cmd();
			if (control_info.State_Bit.WaitRk == 0 &&
				control_info.State_Bit.powerUp == 0)
			{
				// Ctrl_Read_State_Ack(&control_info);
				if (Startflag == StartCmdBegin)
				{
					Task_control_Begin(&control_info);
				}
				if (Startflag == StartCmdStop)
				{
					Task_control_ReqStop(&control_info);
				}
				if (Startflag == StartCmdPause)
				{
					Task_control_ReqPause(&control_info);
				}
			}
			else
			{
				if (Startflag == StartCmdStop)
				{
					control_info.State_Bit.powerUp = 0;
					// control_info.Dev_Mode_Bit.Rkmask = 1;
				}
			}
		}
		else
		{
			/*if (ctrlWaitRk3588() != 0)
			{
				control_info.State_Bit.WaitRk = 1;
			}*/
		}
#ifndef ATM32F43_xx
		control_info.State_Bit.powerUp = 0;
		control_info.State_Bit.WaitRk = 0;
		control_info.Rk3588_Flag.Rflag = 0;
		// control_info.Rk3588_Flag.Lflag = 0;
		Rk3588_Ack_Cmd_Handle(&control_info, 0);
		Rk3588_Ack_Cmd_Handle(&control_info, 1);
#else
		switch (control_info.Dev_Mode_Bit.UARTMODE)
		{
		case 0:
			control_info.State_Bit.powerUp = 0;
			control_info.State_Bit.WaitRk = 0;
			control_info.Rk3588_Flag.Rflag = 0;
			control_info.Rk3588_Flag.Lflag = 0;
			break;
		case 1:
			control_info.Rk3588_Flag.Lflag = 0;
			break;
		case 2:

			break;
		}
		Rk3588_Ack_Cmd_Handle(&control_info, 0);
		Rk3588_Ack_Cmd_Handle(&control_info, 1); //(L) pc using com 1 stm32 using uart1
		//		if (control_info.Dev_Mode_Bit.Rkmask)
		//		{
		//			control_info.State_Bit.powerUp = 0;
		//			control_info.State_Bit.WaitRk = 0;
		//			control_info.Rk3588_Flag.Rflag = 0;
		//			control_info.Rk3588_Flag.Lflag = 0;
		//		}
		//		else
		//			control_info.Rk3588_Flag.Rflag = 0;
#endif
		ControlDelay(10);
		// e.ExitFlag = 1;
	}
}

void controlInit()
{
	Meassage_Center_Add("task");
	Meassage_Center_Add("page1");
	Meassage_Center_Add("Ctrl");
	thread_create(controlfunction, 0, &contrl_thread, 750, "control", 10);
	/*extern void SRV1_Encoder_Init();
	SRV1_Encoder_Init();*/
}
