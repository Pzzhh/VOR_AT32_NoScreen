/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-02-06 16:56:05
 * @FilePath: \USERd:\workfile\椤圭洰3 vor\software\VOR_V2.0\INTERFACE\UI\control\control_VOR.c
 * @Description: 杩欐槸榛樿?よ?剧疆,璇疯?剧疆`customMade`, 鎵撳紑koroFileHeader鏌ョ湅閰嶇疆 杩涜?岃?剧疆: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_CONT.h"
#include "control_Hardware_API.h"
#include "../other/system_function.h"
#include "task_control.h"
#include "control.h"
#include "stdint.h"
#include "control_base.h"
#define use_windows
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "../implement/Slave_Cont_Ctrl.h"
#endif // use_windows
	   // 鏃燯I鎺у埗
// 杩斿洖message

struct
{
	uint32_t time;
	uint32_t RemainTime;
	uint8_t flag_pause;
	uint32_t SetSec;
	float Vel;
} Cont_info;

uint8_t HAL_Slave_CONT_Init(Task_Parameter_Struct *e)
{
#ifndef ATM32F43_xx
	Cont_info.time = ControlGetTick();
	Cont_info.SetSec = e->CONT.Sec;
	Cont_info.flag_pause = 0;
#else
	CONT_Machine_Init(e->CONT.Vel,e->CONT.okr_Vel, e->CONT.Sec * 1000);
#endif // !ATM32F43_xx

	return 1;
}

uint8_t HAL_Slave_CONT_Stop(void)
{
#ifndef ATM32F43_xx
	uint32_t currentSec = (ControlGetTick() - Cont_info.time) / 1000.0f;
	Cont_info.SetSec = currentSec + 1;
#else
	Cont_Machine_Stop();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_CONT_Pause(uint8_t enable)
{
#ifndef ATM32F43_xx
	if (enable)
	{
		Cont_info.flag_pause = 1;
		Cont_info.RemainTime = ControlGetTick() - Cont_info.time;
	}
	else
	{
		Cont_info.time = ControlGetTick() - Cont_info.RemainTime;
		Cont_info.flag_pause = 0;
	}
#else
	Cont_Machine_Pause();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_CONT_Get_State(uint32_t *remainingSec, uint32_t *parcent)
{
#ifndef ATM32F43_xx
	if (Cont_info.flag_pause)
		return Imp_paused;
	uint32_t currentSec = (ControlGetTick() - Cont_info.time) / 1000.0f;
	if (parcent != 0)
		*parcent = currentSec * 100 / Cont_info.SetSec;
	if (currentSec < Cont_info.SetSec)
	{
		*remainingSec = Cont_info.SetSec - currentSec;
		return Imp_running;
	}
	*remainingSec = 0;
	return Imp_finsih;
#else
	uint32_t MillSecReq = 0, CurrentMillSec = 0;
	uint8_t res = Cont_Machine_Get_Count(&MillSecReq, &CurrentMillSec);
	if (parcent != 0 && MillSecReq != 0)
		*parcent = CurrentMillSec * 100 / MillSecReq;
	*remainingSec = MillSecReq - CurrentMillSec;
	return res;
#endif // !ATM32F43_xx
}

static const char *CONT_Driver_Name(const Task_Parameter_Struct *task)
{
	(void)task;
	return "CONT";
}

static const Control_Driver CONT_Driver = {
	.name = CONT_Driver_Name, .start = HAL_Slave_CONT_Init,
	.pause = HAL_Slave_CONT_Pause, .get_state = HAL_Slave_CONT_Get_State,
	.stop = HAL_Slave_CONT_Stop, .next = 0, .on_tick = 0,
	.poll_interval_ms = 50
};

const Control_Driver *CONT_Get_Driver(void) { return &CONT_Driver; }

uint8_t ContControlFunction(Task_Parameter_Struct *task, Task_control_info *e)
{
	const uint32_t begin_time = 10000;
	const uint32_t camWaitTime_s = 5;

	MYPRINTF("\r\n Cont begin");
	MYPRINTF("\r\n");
	// 鎵ц?岄儴鍒?
	MYPRINTF("\r\n");
	Pause_Resume:
	e->UI_para.state = taskinit;
	/*cam rec*/
	Rk3588_Send_Task_Array(e);
	Task_Delay_Msg("Rec After %d", 5);
	uint8_t CAM_State = HAL_CAM_REC_Set(1);
	Task_Delay_Msg("Run After %d", 10);
	/*--cam rec*/
	/*begin motion*/
	HAL_Slave_CONT_Init(task);
	e->UI_para.state = taskruning;
	uint8_t CONT_flag = 1, CamIsStop = 0; // 闃叉?㈠惎鍔ㄦ椂鍊欏彂鐢熸剰澶栬Е鍙?
	int32_t LastCount = -1;
	// motor initial
	uint32_t count, parcent;
	e->State_Bit.reqPause = 0;
	while (CONT_flag)
	{
		CONT_flag = HAL_Slave_CONT_Get_State(&count, &parcent);
		if (LastCount != count && CONT_flag == Imp_running)
		{
			Ctrl_Msg_Printf("%d:CONT Done:%d%%", e->currentCount+1, parcent);
			// HAL_Set_UI_Page1_Msg("Count:%d", count);
			LastCount = count;
		}
		if (e->State_Bit.Exit) // for exit
		{
			Ctrl_Msg_Printf("%d:CONT #A52A2A Terminated#", e->currentCount+1);
			e->UI_para.state = taskTerminal;
			HAL_Slave_CONT_Stop();
		}
		if (e->State_Bit.reqPause)
		{
			if (CONT_flag == Imp_running)
			{
				e->State_Bit.isPause = 1;
				HAL_Slave_CONT_Pause(1);
				HAL_CAM_REC_Set(0);
				e->UI_para.state = taskPause;
                Rk3588_Send_Pause;
				Ctrl_Msg_Printf("%d:CONT Pause", e->currentCount+1);
			}
			else if (CONT_flag == Imp_paused)
			{
				//e->UI_para.state = taskruning;
				Task_Delay_Msg("Resume After %d", 5);
				e->State_Bit.isPause = 0;
				goto Pause_Resume;
			}
			e->State_Bit.reqPause = 0;
		}

		MYPRINTF("%3d", count);
		// wait motor infinsh
		SaftExitDelay(10, 0);

		MYPRINTF("\r");
	}
	Ctrl_Msg_Printf("%d:CONT Done:100%%", e->currentCount+1);
	/*one sec for cam stop*/
	SaftExitDelay(1000, 0);
	if (CamIsStop == 0)
		HAL_CAM_REC_Set(0);
	/*--one sec for cam stop*/
	Ctrl_Msg_Printf("end");
	MYPRINTF("\r\n vor end");
	e->UI_para.state = taskend;
}
