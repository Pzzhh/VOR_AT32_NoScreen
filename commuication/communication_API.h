/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-30 11:26:20
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-31 11:41:10
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\commuication\communication_API.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AEuint8_t HAL_BL_USART_INIT(void)
 */
#ifndef  __communication_API_H
#define __communication_API_H

#ifdef __cplusplus
extern "C"
{
#endif //
#include "stdint.h"

	uint8_t HAL_BL_USART_Server(void);
	void HAL_communication_Init(void);
	int Hal_Rk3588_R_ReadLine(const char* format, ...);
	int Hal_Rk3588_L_ReadLine(const char* format, ...);
	//UART1 right
	int Hal_Rk3588_R_Readarray(char* bytes);
	//UART6 left
	int Hal_Rk3588_L_Readarray(char* bytes);
	int HAL_USB_Isconnect(void);
	
	// USB ACM functions
	uint8_t HAL_USB_ACM_Init(void);
	uint8_t HAL_USB_ACM_Send(uint8_t* data, uint16_t len);
	uint16_t HAL_USB_ACM_Read(uint8_t* buffer, uint16_t maxlen);
	uint8_t HAL_USB_ACM_IsReady(void);
	
	// USB ACM Log functions
	void HAL_USB_Log_Printf(const char* format, ...);
	void HAL_USB_Log_Task_Operation(const char* operation, uint16_t index, const char* details);
	void HAL_USB_Log_Task_Detail(const char* operation, uint16_t index, void* task);
	
	// LVGL Memory Analysis
	void HAL_USB_LVGL_Memory_Analysis(void);
	
	// FreeRTOS Heap Analysis
	void HAL_USB_Heap_Analysis(void);
	
	// Task Stack Analysis
	void HAL_USB_Stack_Analysis(void);
	
	// Full Memory Check
	void HAL_USB_Memory_Full_Check(void);
	
	// Task Priority Diagnosis
	void HAL_USB_Task_Priority_Diagnosis(void);
	
	// Command Processing
	void HAL_USB_Command_Process(const char* cmd);
	void HAL_USB_Command_Handler(void);  // Main loop handler

#ifdef __cplusplus
}
#endif //

#endif // ! __communication_API_H
