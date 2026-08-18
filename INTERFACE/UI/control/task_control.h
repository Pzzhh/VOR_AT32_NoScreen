/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 10:27:18
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-27 10:31:35
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\control\task_control.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef  __task_control_h_
#define __task_control_h_
#include "stdint.h"
#include "../control/control.h"

#ifdef __cplusplus
extern "C"
{
#endif // 
	//void Task_control_handler(Task_control_info* e,Task_Parameter_Struct* TaskArray,uint16_t count);
	uint8_t Task_control_Begin(Task_control_info *e);
	uint8_t Task_control_ReqStop(Task_control_info* e);
	uint8_t Task_control_ReqPause(Task_control_info* e);

	void RK3588_RecordStop_Printf(uint8_t LR);
	void RK3588_ReqRkErrorStatus_Printf(uint8_t LR);
	

#define Rk3588_Send_Task_Array(n)                      \
	{                                               \
		n->Rk3588_Flag.Lflag = SendTaskArray; \
		n->Rk3588_Flag.Rflag = SendTaskArray; \
	}

#define Rk3588_Send_ReqRkErrorStatus(n)                      \
	{                                               \
		n->Rk3588_Flag.Lflag = ReqRKErrorStatus; \
		n->Rk3588_Flag.Rflag = ReqRKErrorStatus; \
	}
	void Task_control_handler(Task_control_info* e);
#ifdef __cplusplus
}
#endif // 

#endif // ! UI_Paremeter_Set_H_
