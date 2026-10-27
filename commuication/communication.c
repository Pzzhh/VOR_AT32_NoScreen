/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:28
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2024-09-23 17:19:07
 * @FilePath: \USERd:\workfile\��Ŀ3 vor\software\VOR_V2.0\commuication\communication.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "communication.h"
#include "../INTERFACE/UI/other/system_function.h"
#include "communication_API.h"
#include "usb_vor_control.h"
#include "usb_motor_telemetry.h"


extern void jump_to_bootloader(void);

void communication_thread()
{
	HAL_communication_Init();
	//USB1_Init();
	while (1)
	{
//		if( HAL_USB_Isconnect())
//				jump_to_bootloader();
		ControlGetTick();
		HAL_BL_USART_Server();
#if defined(ATM32F43_xx) || defined(use_linux)
		// USB�����
		HAL_USB_Command_Handler();
		USB_VOR_Communication_Poll();
#endif
		ControlDelay(10);
	}
}
uint32_t taskhand[2];
void Communication_Init()
{
	thread_create(
		communication_thread,
		0,
		&taskhand[0],
		750,
		"communication",
		11);
#ifdef ATM32F43_xx
	thread_create(
		USB_Motor_Telemetry_Thread,
		0,
		&taskhand[1],
		256,
		"usb_telemetry",
		11);
#endif
	// ControlThreadCreate(
	// 	communication_thread,
	// 	0,
	// 	taskhand,
	// 	"communication",
	// 	256);
}
