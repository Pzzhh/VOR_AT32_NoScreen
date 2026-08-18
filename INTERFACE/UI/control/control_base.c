/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-02-06 16:47:30
 * @FilePath: \USERd:\workfile\é¡¹ç›®3 vor\software\VOR_V2.0\INTERFACE\UI\control\control_VOR.c
 * @Description: è¿™æ˜¯é»˜è?¤è?¾ç½®,è¯·è?¾ç½®`customMade`, æ‰“å¼€koroFileHeaderæŸ¥çœ‹é…ç½® è¿›è?Œè?¾ç½®: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_base.h"
#include "control_Hardware_API.h"
#include "../other/system_function.h"
#include "control.h"
#include "stdint.h"
#define use_windows
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "../implement/Slave_Vor_Ctrl.h"
#endif // use_windows



uint8_t ControlFunction(Task_Parameter_Struct* task, Task_control_info* e, control_cb_struct* cb)
{
	const int camWaitTime_s = 5;
	MYPRINTF("\r\n vor begin");
	MYPRINTF("\r\n");
	MYPRINTF("\r\n");
	if (cb->getstatus == 0 ||
		cb->Init == 0 ||
		cb->Name == 0 ||
		cb->pause == 0 ||
		cb->stop == 0)
		return 0;
Pause_Resume:
	e->UI_para.state = taskinit;
	uint8_t CAM_State = HAL_CAM_REC_Set(1);
	for (int i = 0; i < camWaitTime_s; i++)
	{
		(CAM_State == 0 && i < 2) ?
			Ctrl_Msg_Printf("camera error") :
			Ctrl_Msg_Printf("Run after %ds", camWaitTime_s - i);
		SaftExitDelay(1000, 0);
	}
	/*motor set running configure*/
	cb->Init(task);
	/*notice control thread current state*/
	e->UI_para.state = taskruning;
	/*get vor machine flag*/
	uint8_t machine_flag = 1, CamIsStop = 0;
	int32_t LastCount = -1;
	uint32_t count, parcent;
	e->State_Bit.reqPause = 0;
	while (machine_flag)
	{
		machine_flag = cb->getstatus(&count, &parcent);
		if (LastCount != count && machine_flag == Imp_running)
		{
			Ctrl_Msg_Printf("%d:%s Done:%d%%", e->currentCount, cb->Name(), parcent);
			LastCount = count;
		}
		if (e->State_Bit.Exit) // for exit
		{
			Ctrl_Msg_Printf("%d:%s #A52A2A Terminated#", e->currentCount,cb->Name());
			e->UI_para.state = taskTerminal;
			cb->stop();
		}
		if (e->State_Bit.reqPause == 1)
		{
			if (machine_flag == Imp_running)
			{
				cb->pause(1);
				Ctrl_Msg_Printf("%d:%s Pause", e->currentCount,cb->Name());
				e->UI_para.state = taskPause;
				Rk3588_Send_Pause;
				HAL_CAM_REC_Set(0);
			}
			else if (machine_flag == Imp_paused)
			{
				//e->UI_para.state = taskruning;
				goto Pause_Resume;
			}
			e->State_Bit.reqPause = 0;
		}
		MYPRINTF("%3d", count);
		// wait motor infinsh
		SaftExitDelay(50, 0);
	}
	if (e->State_Bit.Exit == 0)
		Ctrl_Msg_Printf("%d:%s Done:100%%", e->currentCount, ModeStr(task));
	/*one sec for cam stop*/
	SaftExitDelay(1000, 0);
	if (CamIsStop == 0)
		CAM_State = HAL_CAM_REC_Set(0);
	if (CAM_State == 0)
	{
		Ctrl_Msg_Printf("CAM ERROR");
	}
	/*--one sec for cam stop*/
	e->UI_para.state = taskend;
	return 0;
}
