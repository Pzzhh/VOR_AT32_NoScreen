/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 10:26:50
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-03-28 20:02:31
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\control\task_control.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "task_control.h"
#include "../other/system_function.h"
#include "../Task/task_info_struct.h"
#include "control_Hardware_API.h"
#include "stdint.h"

#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>

#else
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "control_VOR.h"
#include "control_CONT.h"
#include "control_OVAR.h"
#include "control_VHIT.h"
#include "control_TC.h"
#include "control_ZERO.h"
#include "control_base.h"
#include "../../../commuication/usb_vor_control.h"
#include "../../../commuication/usb_control_base.h"

#include <stdarg.h>

Task_Parameter_Struct TaskArray[Task_ARY_MAX];
typedef struct
{
	uint16_t currentTaskIndex;
	uint32_t remainingTime;
} Task_Control_Status;
// static char msg_buffer[50];
#define RKPrint(...)                                                        \
	{                                                                       \
		(LR) ? Rk3588_R_Printf(__VA_ARGS__) : Rk3588_L_Printf(__VA_ARGS__); \
	}
static Task_control_info *control;
extern uint8_t Ctrl_Msg_Printf(const char *format,
							   ...);
extern void Rk3588_R_Printf(const char *strOutputString, ...);
extern void Rk3588_L_Printf(const char *strOutputString, ...);

#define Rk3588_Task_Cancel                   \
	{                                        \
		control->Rk3588_Flag.Lflag = Cancel; \
		control->Rk3588_Flag.Rflag = Cancel; \
	}

void PrintfInfo(Task_Parameter_Struct *e, char *mousename, uint8_t LR, uint16_t index)
{
	static const char vorstrList[][10] = {"VOR", "OKR", "VOR_OKR", "VOR_OKRR"};
	static char timeStamp[20] = "20210101093100";

	const char *vorStr = vorstrList[e->VOR.ExMode];
	timetyped time = {0};
	Message_Center_Read_prinft("Ctrl", &time, sizeof(time), "ReqTime");
	sprintf(timeStamp, "%04d%02d%02d%02d%02d%02d",
			time.date.year, time.date.month, time.date.date,
			time.date.hour, time.date.min, time.date.sec);
	index++;
	switch (e->mode)
	{
	case Task_VOR:
		RKPrint("%s_%s_%s_T%dV%dF%02d_%d", timeStamp, mousename, vorStr, e->VOR.Counter,
				(int)e->VOR.Vel, (int)(e->VOR.Freq * 10.0f), index);
		break;
	case Task_Continue:
		RKPrint("%s_%s_CONT_S%dV%d_%d", timeStamp, mousename, e->CONT.Sec,
				(int)e->CONT.Vel, index);
		break;
	case Task_OVAR:
		RKPrint("%s_%s_OVAR_S%dV%dI%02d_%d", timeStamp, mousename, e->OVAR.Sec, (int)e->OVAR.Vel, (int)(e->OVAR.Inc_Degree), index);
		break;
	case Task_VHIT:
		RKPrint("%s_%s_VHIT_T%d%s_%d", timeStamp, mousename, e->VHIT.Counter, (e->VHIT.IsCW) ? "CW" : "CCW", index);
		break;
	case Task_TC:
		RKPrint("%s_%s_TC_S%dV%d_%d", timeStamp, mousename, e->TC.Counter, e->TC.Vel, index);
		break;

	default:
		break;
	}
}

void RK3588_SendTaskInfo(uint16_t tasksize, uint8_t LR)
{
	Task_Parameter_Struct *e;
	char *mousename = (char *)malloc(50);
	mousename[0] = 0;
	HAL_Get_UI_MouseName(mousename, 50);
	if (mousename[0] == 0)
		strncpy(mousename, "unnamed", 50);
	(LR) ? Rk3588_R_Printf("filename:") : Rk3588_L_Printf("filename:");
	/*for (int i = 0; i < tasksize; i++)
	{*/
	e = &TaskArray[tasksize];
	PrintfInfo(e, mousename, LR,tasksize);
	//}
	(LR) ? Rk3588_R_Printf("\r\n") : Rk3588_L_Printf("\r\n");
	free(mousename);
}

void RK3588_RecordStop_Printf(uint8_t LR)
{
	(LR) ? Rk3588_R_Printf("recordingEnd") : Rk3588_L_Printf("recordingEnd");
}

void RK3588_ReqRkErrorStatus_Printf(uint8_t LR)
{
	(LR) ? Rk3588_R_Printf("requiredstatus") : Rk3588_L_Printf("requiredstatus");
}

void RK3588_End_Printf(uint8_t LR)
{
	(LR) ? Rk3588_R_Printf("finish") : Rk3588_L_Printf("finish");
}

void RK3588_terminal_Printf(uint8_t LR)
{
	(LR) ? Rk3588_R_Printf("cancel") : Rk3588_L_Printf("cancel");
}

void RK3588_SendTestCmd()
{
	Rk3588_R_Printf("UART Test\n");
	Rk3588_L_Printf("UART Test\n");
}

void RK3588_Initial_Printf(void)
{
	Rk3588_R_Printf("clear\n");
	Rk3588_L_Printf("clear\n");
}

void RK3588_pause_Printf(uint8_t LR)
{
	(LR) ? Rk3588_R_Printf("pause") : Rk3588_L_Printf("pause");
}

void RK3588_interrupt_Printf(void)
{
	Rk3588_R_Printf("interrupt");
	Rk3588_L_Printf("interrupt");
}

void task_interval_handle(int index)
{
	const uint32_t waitMillSec = 10000, delayMS = 100;
	uint16_t count = waitMillSec / delayMS - 1;
	// int32_t time = ControlGetTick(), remainingTime = 0;
	//  uint16_t len = sprintf(msg_buffer, "Interval: ID:%d", index);
	//  Message_Center_Send("PAGE1", 0, msg_buffer, len);
	/*  Message_Center_Send_prinft(
		  "PAGE1", 0, 0,
		  "Interval: ID:%d", index);*/
	/*HAL_Set_UI_Page1_Msg("Interval: ID:%d", index);*/
	while (count)
	{
		Ctrl_Msg_Printf("Processing... %ds", count / 10 + 1);
		count--;
		if (control->State_Bit.Exit) // for exit
		{
			return;
		}
		ControlDelay(delayMS);
	}
}

uint8_t Task_control_Begin(Task_control_info *e)
{
	if (e->controlOwner == CONTROL_OWNER_USB)
		return 0;
	if (e->State_Bit.IsRunning == 1)
	{
		return 0;
	}
	e->taskCount = HAL_Task_GetArray(TaskArray, 10);
	if (e->taskCount == 0 || e->taskCount > 20)
		return 0;
	e->taskArray = TaskArray;
	e->State_Bit.flag = 0;
	e->controlOwner = CONTROL_OWNER_UI;
	e->State_Bit.isRun = 1;
	return 1;
}

uint8_t Task_control_ReqStop(Task_control_info *e)
{
	if (e->controlOwner == CONTROL_OWNER_USB)
		return 0;
	if (e->State_Bit.IsRunning == 0)
	{
		return 0;
	}
	e->State_Bit.reqPause = 0;
	e->State_Bit.Exit = 1;
	return 1;
}

uint8_t Task_control_ReqPause(Task_control_info *e)
{
	if (e->controlOwner == CONTROL_OWNER_USB)
		return 0;
	if (e->State_Bit.IsRunning == 0)
	{
		return 0;
	}
	e->State_Bit.reqPause = !e->State_Bit.reqPause;
	return 1;
}

void Task_Delay_Msg(char *format, int sec)
{
	for (int i = 0; i < sec; i++)
	{
		Ctrl_Msg_Printf(format, sec - i);
		SaftExitDelay(1000, 0);
	}
}

void Task_control_handler(Task_control_info *e)
{
	uint8_t Rk3588_ReadyFlag_1 = 0, Rk3588_ReadyFlag_2 = 0;
	uint8_t skip_loop = 0;
	USB_VOR_Command usb_start_command;
	if (e == 0)
		return;
	control = e;
	e->State_Bit.isRun = 0;
	e->State_Bit.WaitRk = 0;
	e->controlOwner = CONTROL_OWNER_NONE;
	Control_Preview_Init();
	Ctrl_Msg_Printf(" ");
	//	ZEROControlFunction(1);
	// HAL_Beep_Enable(500);
	// uint16_t len = 0;
BEGIN_POS:
	// get all task need to implement
	e->State_Bit.isRun = 0;
	e->State_Bit.IsRunning = 0;
	while (1)
	{
		if (e->controlOwner == CONTROL_OWNER_USB &&
			USB_VOR_Task_Peek_Start(&usb_start_command))
		{
			TaskArray[0] = usb_start_command.task;
			goto USB_RUNNING;
		}
		if (e->State_Bit.isRun == 1)
		{
			e->State_Bit.flag = 0;
			e->Error_Bit.flag = 0;
			if (e == 0 || e->taskArray == 0)
			{
				e->Error_Bit.paraError = 1;
				continue;
			}
			break;
		}
		SaftExitDelay(100, 0);
	}
	goto UI_RUNNING;

USB_RUNNING:
	e->State_Bit.flag = 0;
	e->Error_Bit.flag = 0;
	e->State_Bit.IsRunning = 1;
	e->State_Bit.isPause = 0;
	e->currentCount = 0;
	e->taskArray = TaskArray;
	e->taskCount = 1;
	e->UI_para.state = taskInv;

	USB_VOR_Task_Set_Mode(TaskArray[0].mode);
	const Control_Driver *usb_driver = 0;
	switch (TaskArray[0].mode)
	{
	case Task_VOR:      usb_driver = VOR_Get_Driver(); break;
	case Task_Continue: usb_driver = CONT_Get_Driver(); break;
	case Task_OVAR:     usb_driver = OVAR_Get_Driver(); break;
	case Task_VHIT:     usb_driver = VHIT_Get_Driver(); break;
	case Task_TC:       usb_driver = TC_Get_Driver(); break;
	default: break;
	}

	if (usb_driver != 0)
		(void)USB_Control_Run(&TaskArray[0], e, usb_driver);
	else
	{
		(void)USB_VOR_Task_Take_Start(&usb_start_command);
		e->Error_Bit.paraError = 1;
		e->UI_para.state = taskTerminal;
		USB_VOR_Task_Reply(&usb_start_command, 0,
						   CONTROL_RESULT_UNSUPPORTED_MODE,
						   "UNSUPPORTED_MODE");
		USB_VOR_Task_Update(USB_VOR_STATE_ERROR,
						CONTROL_RESULT_UNSUPPORTED_MODE, "ERROR", 0, 0);
	}

	/* A USB Start owns exactly one task, then returns to BEGIN_POS. */
	e->State_Bit.IsRunning = 0;
	e->State_Bit.isPause = 0;
	e->State_Bit.flag = 0;
	e->controlOwner = CONTROL_OWNER_NONE;
	e->UI_para.state = taskInv;
	USB_VOR_Task_Update(USB_VOR_STATE_IDLE, CONTROL_RESULT_OK,
						"IDLE", 0, 0);
	USB_VOR_Task_Reset();
	goto BEGIN_POS;

UI_RUNNING:
	e->State_Bit.IsRunning = 1;
	e->State_Bit.isPause = 0;
	e->Rk3588_Flag.WifiMsgFlag = 0;
	e->Rk3588_Flag.isWifiCauseStop = 0;
	e->currentCount = 0;
	skip_loop = 0;

	uint16_t task_size = e->taskCount;
	int i = 0;
	Ctrl_Msg_Printf(" ");
	HAL_Outage_Log(1, 0);
	if (e->StartWithIndex < task_size)
	{
		i = e->StartWithIndex;
		e->StartWithIndex = 0;
		if (i == 0)
			ZEROControlFunction(1);
	}
	else
	{
		e->StartWithIndex = 0;
		goto BEGIN_POS;
	}

	for (; i < task_size; i++)
	{
		HAL_Outage_Log(1, i);
		Task_Parameter_Struct *task = &e->taskArray[i];
		e->currentCount = i;
		e->UI_para.state = taskInv;

		Control_Result task_result = CONTROL_RESULT_OK;
		switch (task->mode)
		{
		case Task_VOR:
			task_result = (Control_Result)VorControlFunction(task, e);
			break;
		default:
			task_result = CONTROL_RESULT_UNSUPPORTED_MODE;
			e->Error_Bit.paraError = 1;
			e->UI_para.state = taskTerminal;
			Control_Preview_Set_Unsupported(i, task->mode);
			Ctrl_Msg_Printf("%d:Mode %d #A52A2A Unsupported#",
						 i + 1, task->mode);
			break;
		}
		if (task_result != CONTROL_RESULT_OK)
		{
			skip_loop = 1;
			break;
		}
		Rk3588_Send_RecordStop;
		e->State_Bit.isPause = 0;
		if (e->Dev_Mode_Bit.Zero == 0)
			ZEROControlFunction(0);
		Ctrl_Msg_Printf(" ");
		SaftExitDelay(1000, 0);
		Rk3588_Send_ReqRkErrorStatus;
		task_interval_handle(0);
		if (e->Rk3588_Flag.isUsingBlock)
		{
			if (e->Rk3588_Flag.WifiMsgFlag != 3)
			{
				uint8_t LE = e->Rk3588_Flag.LerrorCode, RE = e->Rk3588_Flag.RerrorCode;
				HAL_Beep_Enable(1000);
				Message_Center_Send_prinft("page1", 0,
										   0,
										   "NoticeWifierror %d %d %d %d", e->Rk3588_Flag.WifiMsgFlag, i,
										   LE, RE);
				HAL_USB_Log_Printf("\n[%d] task status:%d %d", ControlGetTick(), LE, RE);
				skip_loop = 1;
				break;
			}
			e->Rk3588_Flag.WifiMsgFlag = 0;
		}
		if (e->State_Bit.Exit)
		{
			Rk3588_Task_Cancel;
			break;
		}
	}
	Ctrl_Msg_Printf("Centering");
	HAL_Outage_Log(0, 0);
	// len = sprintf(msg_buffer, "End");
	// Message_Center_Send("PAGE1", 0, msg_buffer, len);
	/*HAL_Set_UI_Page1_Msg("End");*/
	uint8_t exitflag = e->State_Bit.Exit;
	e->State_Bit.IsRunning = 0;
	e->controlOwner = CONTROL_OWNER_NONE;
	e->State_Bit.flag = 0;
	e->Error_Bit.flag = 0;
	if (exitflag == 0 &&
		e->Dev_Mode_Bit.LoopTest == 1 && skip_loop == 0)
	{
		SaftExitDelay(5000, 0);
		Message_Center_Send_prinft("Ctrl", 0,
								   0,
								   "ReqStrat");
	}
	goto BEGIN_POS;
}
