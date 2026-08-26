/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-02-10 15:38:33
 * @FilePath: \USERd:\workfile\椤圭洰3 vor\software\VHIT_V2.0\INTERFACE\UI\control\control_VHIT.c
 * @Description: 杩欐槸榛樿?よ?剧疆,璇疯?剧疆`customMade`, 鎵撳紑koroFileHeader鏌ョ湅閰嶇疆 杩涜?岃?剧疆: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_VHIT.h"
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
#include "../implement/Slave_VHIT_Ctrl.h"
#endif // use_windows
	   // 鏃燯I鎺у埗
// 杩斿洖message
const int singalTick = 1 * 1000;
#define ModeName "VHIT"
struct
{
	uint32_t time;
	uint32_t currentCNT;
	uint8_t flag_pause;
	uint32_t SetCount;
	uint8_t isSingalFinish;
} vhit_info;

uint8_t HAL_Slave_VHIT_Init(Task_Parameter_Struct *e)
{
#ifndef ATM32F43_xx
	vhit_info.time = ControlGetTick();
	vhit_info.SetCount = e->VHIT.Counter;
	vhit_info.flag_pause = 0;
#else
	if (e->mode == Task_VHIT)
		VHIT_Machine_Init(e->VHIT.Counter, e->VHIT.IsCW);
#endif // !ATM32F43_xx

	return 1;
}

uint8_t HAL_Slave_VHIT_Stop(void)
{
#ifndef ATM32F43_xx
	uint32_t currentCNT = (ControlGetTick() - vhit_info.time) / singalTick;
	vhit_info.SetCount = currentCNT + 1;
#else
	VHIT_Machine_Stop();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_VHIT_Pause(uint8_t enable)
{
#ifndef ATM32F43_xx
	/* if (enable)
	 {
		 vhit_info.flag_pause = 1;
		 vhit_info.RemainTime = ControlGetTick() - vhit_info.time;
	 }
	 else
	 {
		 vhit_info.time = ControlGetTick() - vhit_info.RemainTime;
		 vhit_info.flag_pause = 0;
	 }*/
#else
	VHIT_Machine_Pause();
#endif // !ATM32F43_xx
	return 1;
}

uint8_t HAL_Slave_VHIT_Get_State(uint32_t *remainingCount, uint32_t *parcent)
{
#ifndef ATM32F43_xx
	if (vhit_info.flag_pause)
		return Imp_paused;
	uint32_t currentTick = (ControlGetTick() - vhit_info.time);
	if (currentTick > singalTick &&
		vhit_info.isSingalFinish == 0)
	{
		currentTick = singalTick;
		vhit_info.currentCNT++;
		vhit_info.isSingalFinish = 1;
	}
	int p = (currentTick * (100 / vhit_info.SetCount)) / singalTick;
	if (parcent != 0)
		*parcent = vhit_info.currentCNT * 100 / vhit_info.SetCount;
	if (vhit_info.currentCNT < vhit_info.SetCount)
	{
		*remainingCount = vhit_info.SetCount - vhit_info.currentCNT;
		return Imp_running;
	}
	*remainingCount = 0;
	return Imp_finsih;
#else
	uint32_t counterReq = 0, CurrentCount = 0;
	uint8_t res = VHIT_Machine_Get_Count(&counterReq, &CurrentCount);
	if (parcent != 0 && counterReq != 0)
		*parcent = CurrentCount * 100 / counterReq;
	*remainingCount = counterReq - CurrentCount;
	return res;
#endif // !ATM32F43_xx
}
uint8_t HAL_Slave_VHIT_Next()
{
#ifndef ATM32F43_xx
	if (vhit_info.isSingalFinish)
	{
		vhit_info.isSingalFinish = 0;
		vhit_info.time = ControlGetTick();
	}
#else
	VHIT_Machine_Next();
#endif
	return 1;
}

static const char *VHIT_Driver_Name(const Task_Parameter_Struct *task)
{
	(void)task;
	return "VHIT";
}

static const Control_Driver VHIT_Driver = {
	.name = VHIT_Driver_Name, .start = HAL_Slave_VHIT_Init,
	.pause = 0, .get_state = HAL_Slave_VHIT_Get_State,
	.stop = HAL_Slave_VHIT_Stop, .next = HAL_Slave_VHIT_Next, .on_tick = 0,
	.poll_interval_ms = 50
};

const Control_Driver *VHIT_Get_Driver(void) { return &VHIT_Driver; }

uint8_t VHITControlFunction(Task_Parameter_Struct *task, Task_control_info *e)
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
	Message_Center_Send_prinft("page1", 0, 0, "VHIT_NEXT_INIT");
	uint8_t CAM_State = HAL_CAM_REC_Set(1);
	Task_Delay_Msg("Run After %d", 10);
	/*motor set running configure*/
	HAL_Slave_VHIT_Init(task);
	/*notice control thread current state*/
	e->UI_para.state = taskruning;
	/*get vor machine flag*/
	uint8_t VHIT_machine_flag = 1, CamIsStop = 0;
	int32_t LastCount = -1;
	uint32_t count, parcent;
	e->State_Bit.reqPause = 0;
	/*waiting vor machine finish*/
	while (VHIT_machine_flag)
	{
		VHIT_machine_flag = HAL_Slave_VHIT_Get_State(&count, &parcent);
		if (LastCount != count && VHIT_machine_flag == Imp_running)
		{
			Ctrl_Msg_Printf("%d:%s Done:%d%%", e->currentCount+1, ModeName, parcent);
			if (LastCount != -1)
				Message_Center_Send_prinft("page1", 0, 0, "VHIT_NEXT_ENABLE");
			LastCount = count;
		}
		if (e->State_Bit.VHIT_Next)
		{
			e->State_Bit.VHIT_Next = 0;
			Message_Center_Send_prinft("page1", 0, 0, "VHIT_NEXT_DISABLE");
			HAL_Slave_VHIT_Next();
		}
		if (e->State_Bit.Exit) // for exit
		{
			e->UI_para.state = taskTerminal;
			Ctrl_Msg_Printf("%d:%s #A52A2A Terminated#", e->currentCount+1, ModeName);
			HAL_Slave_VHIT_Stop();
		}
		// if (e->State_Bit.reqPause == 1)
		// {
		// 	if (VHIT_machine_flag == Imp_running)
		// 	{
		// 		HAL_Slave_VHIT_Pause(1);
		// 		Ctrl_Msg_Printf("%d:%s Pause", e->currentCount, ModeName);
		// 		Rk3588_Send_Pause;
		// 		// if (CamIsStop == 0)
		// 		// 	HAL_CAM_REC_Set(1);
		// 		// CamIsStop = 1;
		// 		HAL_CAM_REC_Set(1);
		// 	}
		// 	else if (VHIT_machine_flag == Imp_paused)
		// 	{
		// 		goto Pause_Resume;
		// 		// Ctrl_Msg_Printf("%d:%s Done:%d%%", e->currentCount, ModeStr(task), parcent);
		// 		// HAL_Slave_VHIT_Pause(0);
		// 		// HAL_CAM_REC_Set(1);
		// 	}
		// 	e->State_Bit.reqPause = 0;
		// }
		MYPRINTF("%3d", count);
		// wait motor infinsh
		SaftExitDelay(50, 0);

		MYPRINTF("\r");
	}
	if (e->State_Bit.Exit == 0)
		Ctrl_Msg_Printf("%d:%s Done:100%%", e->currentCount+1, ModeName);
	/*one sec for cam stop*/
	SaftExitDelay(1000, 0);
	CAM_State = HAL_CAM_REC_Set(0);
	if (CAM_State == 0)
	{
		Ctrl_Msg_Printf("CAM ERROR");
	}
	Message_Center_Send_prinft("page1", 0, 0, "VHIT_NEXT_DEINIT");
	/*--one sec for cam stop*/
	MYPRINTF("\r\n vor end");
	e->UI_para.state = taskend;
	return 0;
}
