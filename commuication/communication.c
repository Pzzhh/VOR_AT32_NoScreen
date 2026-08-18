/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:28
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2024-09-23 17:19:07
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\commuication\communication.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "communication.h"
#include "../INTERFACE/UI/other/system_function.h"
#include "communication_API.h"


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
		// USB命令处理
		HAL_USB_Command_Handler();
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
	// ControlThreadCreate(
	// 	communication_thread,
	// 	0,
	// 	taskhand,
	// 	"communication",
	// 	256);
}
