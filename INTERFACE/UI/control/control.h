/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-12 17:41:30
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\control\control.h
 * @Description: 这是默�?��?�置,请�?�置`customMade`, 打开koroFileHeader查看配置 进�?��?�置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __control_h_
#define __control_h_
#ifdef __cplusplus
extern "C"
{
#endif //
#include "stdint.h"
#include "../Task/task_info_struct.h"
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#endif // use_windows

#define control_cb_array_size 10
#define CmdCheck(in, ref) strncmp(in, ref, sizeof(ref) - 1)
	typedef enum
	{
		taskinit,
		taskruning,
		taskPause,
		taskTerminal,
		taskend,
		taskInv,
	} task_runing;
	typedef enum
	{
		CONTROL_OWNER_NONE = 0,
		CONTROL_OWNER_UI,
		CONTROL_OWNER_USB
	} Control_Owner;
	typedef struct
	{
		union
		{
			uint16_t flag;
			struct
			{
				uint8_t H_init:1;//hardware init
				uint8_t isRun : 1;	//trager
				uint8_t WaitRk : 1;
				uint8_t IsRunning : 1;	//flag
				uint8_t Exit : 1;
				uint8_t VHIT_Next : 1;
				uint8_t finish : 1;
				uint8_t reqPause : 1;
				uint8_t isPause : 1;
				uint8_t powerUp : 2;
			};
		} State_Bit;
		union
		{
			uint16_t flag;
			struct
			{
				uint8_t H_init : 1;//hardware init
			};
		} State2_Bit;
		struct
		{
			enum
			{
				Rk3588_Uart_Idle,
				SendTaskArray,
				Cancel,
				Pause,
				RecordStop,
				ReqRKErrorStatus,
			} Rk3588_Flag_Typed;
			uint8_t Lflag;
			uint8_t Rflag;
			uint8_t isUsingBlock;
			uint8_t LerrorCode;
			uint8_t RerrorCode;
			uint8_t WifiMsgFlag;		//l-> 01'b2 R->10'b2 
			uint8_t isWifiCauseStop; 
		} Rk3588_Flag;
		union
		{
			uint8_t flag;
			struct
			{
				uint8_t paraError : 1;
			};
		} Error_Bit;
		union
		{
			uint32_t flag;
			struct
			{
				uint8_t LoopTest : 1;
				uint8_t UARTMODE : 4;//0: disable serial 1:rk 2:h618 uart1
				uint8_t Use_Cam : 1;	//
				uint8_t C610 : 1;
				uint8_t Inc_DIR : 1;
				uint8_t Zero : 1;
			};
		} Dev_Mode_Bit;
#ifndef ATM32F43_xx
		HANDLE hThread;
#endif // use_windows
		struct
		{
			uint32_t sec;
			task_runing state;
		} UI_para;
		uint8_t ismessageUpdata;
		uint8_t message[50];
		uint8_t isWifimessageUpdata;
		//uint8_t isReqWifimessage;
		uint8_t Wifimessage[50];
		Task_Parameter_Struct* taskArray;
		uint16_t taskCount;
		uint16_t currentCount;
		uint16_t taskParcentage;
		uint8_t isExpired;
		uint8_t StartWithIndex;
		volatile Control_Owner controlOwner;
#ifdef ATM32F43_xx

#endif // ATM32F43_xx

	} Task_control_info;
	void controlInit(void);
	// 0 代表无返回信�?
	typedef uint8_t(*UI_function)(uint8_t* cmd, uint8_t* meassage, Task_control_info* e);
	typedef struct
	{
		UI_function cb;
		uint8_t message[50];
	} UI_Function_struct;
	uint8_t Control_Set_cb(UI_Function_struct* function_array,
		uint16_t index,
		UI_function cb);
	uint8_t Control_Remove_cb(UI_Function_struct* function_array,
		uint16_t index,
		UI_function cb);

	uint8_t Ctrl_Msg_Printf(const char* format,
		...);

	extern UI_Function_struct control_cb_array[control_cb_array_size];
#ifdef __cplusplus
}
#endif //

#endif // ! UI_Paremeter_Set_H_
