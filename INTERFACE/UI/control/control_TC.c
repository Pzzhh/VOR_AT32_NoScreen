/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-02-06 16:47:30
 * @FilePath: \USERd:\workfile\椤圭洰3 vor\software\TC_V2.0\INTERFACE\UI\control\control_TC.c
 * @Description: 杩欐槸榛樿?よ?剧疆,璇疯?剧疆`customMade`, 鎵撳紑koroFileHeader鏌ョ湅閰嶇疆 杩涜?岃?剧疆: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_TC.h"
#include "control_Hardware_API.h"
#include "../other/system_function.h"
#include "control.h"
#include "task_control.h"
#include "stdint.h"
#include "control_base.h"
#define use_windows
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "../implement/Slave_TC_Ctrl.h"
#endif // use_windows
 // 鏃燯I鎺у埗
// 杩斿洖message
const int singalCount = 1 * 1000;
struct
{
	uint32_t time;
	uint32_t RemainTime;
	uint8_t flag_pause;
	uint32_t SetCount;
	float Vel;
} tc_info;

uint8_t HAL_Slave_TC_Init(Task_Parameter_Struct* e)
{
#ifndef ATM32F43_xx
	tc_info.time = ControlGetTick();
	tc_info.SetCount = e->TC.Counter;
	tc_info.flag_pause = 0;
#else
	if (e->mode == Task_TC)
		TC_Machine_Init(e->TC.Counter, e->TC.Vel);
#endif // !ATM32F43_xx

	return 1;
}

uint8_t HAL_Slave_TC_Stop(void)
{
#ifndef ATM32F43_xx
	uint32_t currentCount = (ControlGetTick() - tc_info.time) / singalCount;
	tc_info.SetCount = currentCount + 1;
#else
	TC_Machine_Stop();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_TC_Pause(uint8_t enable)
{
#ifndef ATM32F43_xx
	if (enable)
	{
		tc_info.flag_pause = 1;
		tc_info.RemainTime = ControlGetTick() - tc_info.time;
	}
	else
	{
		tc_info.time = ControlGetTick() - tc_info.RemainTime;
		tc_info.flag_pause = 0;
	}
#else
	TC_Machine_Pause();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_TC_Get_State(uint32_t* remainingCount, uint32_t* parcent)
{
#ifndef ATM32F43_xx
	if (tc_info.flag_pause)
		return Imp_paused;
	uint32_t currentCount = (ControlGetTick() - tc_info.time) / singalCount;
	if (parcent != 0)
		*parcent = currentCount * 100 / tc_info.SetCount;
	if (currentCount < tc_info.SetCount)
	{
		*remainingCount = tc_info.SetCount - currentCount;
		return Imp_running;
	}
	*remainingCount = 0;
	return Imp_finsih;
#else
	uint32_t counterReq = 0, CurrentCount = 0;
	uint8_t res = TC_Machine_Get_Count(&counterReq, &CurrentCount);
	if (parcent != 0 && counterReq != 0)
		*parcent = CurrentCount * 100 / counterReq;
	*remainingCount = counterReq - CurrentCount;
	return res;
#endif // !ATM32F43_xx
}

static const char *TC_Driver_Name(const Task_Parameter_Struct *task)
{
	(void)task;
	return "TC";
}

static const Control_Driver TC_Driver = {
	.name = TC_Driver_Name, .start = HAL_Slave_TC_Init,
	.pause = HAL_Slave_TC_Pause, .get_state = HAL_Slave_TC_Get_State,
	.stop = HAL_Slave_TC_Stop, .next = 0, .on_tick = 0,
	.poll_interval_ms = 50
};

const Control_Driver *TC_Get_Driver(void) { return &TC_Driver; }



uint8_t TcControlFunction(Task_Parameter_Struct* task, Task_control_info* e)
{
	const int camWaitTime_s = 5;
	MYPRINTF("\r\n vor begin");
	MYPRINTF("\r\n");
	// 鎵ц?岄儴鍒?
	MYPRINTF("\r\n");
Pause_Resume:
	e->UI_para.state = taskinit;
	Rk3588_Send_Task_Array(e);
	Task_Delay_Msg("Rec After %d", 5);
	uint8_t CAM_State = HAL_CAM_REC_Set(1);
	Task_Delay_Msg("Run After %d", 10);
	/*motor set running configure*/
	HAL_Slave_TC_Init(task);
	/*notice control thread current state*/
	e->UI_para.state = taskruning;
	/*get vor machine flag*/
	uint8_t TC_machine_flag = 1, CamIsStop = 0;
	int32_t LastCount = -1;
	uint32_t count, parcent;
	e->State_Bit.reqPause = 0;
	e->State_Bit.VHIT_Next = 0;
	/*waiting vor machine finish*/
	while (TC_machine_flag)
	{
		TC_machine_flag = HAL_Slave_TC_Get_State(&count, &parcent);
		if (LastCount != count && TC_machine_flag == Imp_running)
		{
			Ctrl_Msg_Printf("%d:%s Done:%d%%", e->currentCount + 1, "TC", parcent);
			LastCount = count;
		}
		if (e->State_Bit.Exit) // for exit
		{
			Ctrl_Msg_Printf("%d:%s #A52A2A Terminated#", e->currentCount + 1, "TC");
			e->UI_para.state = taskTerminal;
			HAL_Slave_TC_Stop();
		}
		if (e->State_Bit.reqPause == 1)
		{
			if (TC_machine_flag == Imp_running)
			{
				HAL_Slave_TC_Pause(1);
				Ctrl_Msg_Printf("%d:%s Pause", e->currentCount + 1, "TC");
				Rk3588_Send_Pause;
				e->UI_para.state = taskPause;
				e->State_Bit.isPause = 1;
				// if (CamIsStop == 0)
				// 	HAL_CAM_REC_Set(1);
				// CamIsStop = 1;
				HAL_CAM_REC_Set(0);
			}
			else if (TC_machine_flag == Imp_paused)
			{
				e->State_Bit.isPause = 0;
				Task_Delay_Msg("Resume After %d", 5);
				//e->UI_para.state = taskruning;
				goto Pause_Resume;
				//Ctrl_Msg_Printf("%d:%s Done:%d%%", e->currentCount,"TC", parcent);
				//HAL_Slave_TC_Pause(0);
				//HAL_CAM_REC_Set(1);
			}
			e->State_Bit.reqPause = 0;
		}
		MYPRINTF("%3d", count);
		// wait motor infinsh
		SaftExitDelay(50, 0);

		MYPRINTF("\r");
	}
	if (e->State_Bit.Exit == 0)
		Ctrl_Msg_Printf("%d:%s Done:100%%", e->currentCount + 1, "TC");
	/*one sec for cam stop*/
	SaftExitDelay(1000, 0);
	if (CamIsStop == 0)
		CAM_State = HAL_CAM_REC_Set(0);
	if (CAM_State == 0)
	{
		Ctrl_Msg_Printf("CAM ERROR");
	}
	/*--one sec for cam stop*/
	MYPRINTF("\r\n vor end");
	e->UI_para.state = taskend;
	return 0;
}
