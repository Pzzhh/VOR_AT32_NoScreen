/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-23 16:30:52
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-23 06:04:43
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\other\system_function.c
 * @Description: 这是默�?��?�置,请�?�置`customMade`, 打开koroFileHeader查看配置 进�?��?�置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "system_function.h"
#include "string.h"
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
// #include "FreeRTOS.h"
// #include "../FREERTOS/source/include/task.h"
#include "FreeRTOS.h"
#include "stdarg.h"
#include "task.h"
#endif // use_windows

void ControlDelay(uint32_t ms)
{
#ifndef ATM32F43_xx
	Sleep(ms);
#else
	vTaskDelay(pdMS_TO_TICKS(ms));
#endif // use_windows
}

void EnterCrital()
{
#ifdef ATM32F43_xx
	taskENTER_CRITICAL();
#endif
}

void ExitCrital()
{
#ifdef ATM32F43_xx
	taskEXIT_CRITICAL();
#endif
}

void SaftExitDelay(uint32_t ms, uint8_t e)
{
	if (e)
	{
#ifndef ATM32F43_xx
		ExitThread(0);
#else
		vTaskDelete(0);
#endif // use_windows
	}
#ifndef ATM32F43_xx
	Sleep(ms);
#else
	vTaskDelay(pdMS_TO_TICKS(ms));
#endif // use_windows
}


// void* message_malloc(size_t size)
// {
// 	void* pt = malloc(size);
// 	return pt;
// }

// void* system_malloc(size_t size)
// {
// 	void* pt = malloc(size);
// 	return pt;
// }

uint32_t ControlGetTick()
{
#ifndef ATM32F43_xx
	static DWORD BeginTick = 0;
	if (BeginTick == 0)
		BeginTick = GetTickCount();
	uint32_t tick = GetTickCount() - BeginTick;
	return tick;

#else
	uint32_t tick = xTaskGetTickCount() / pdMS_TO_TICKS(1); //issue
	return tick;
#endif
}

void OutputDebugPrintf(const char* strOutputString, ...)
{
	static uint8_t isbusy = 0;
	uint8_t* pt = &isbusy;
	while (*pt)
	{
	}
	isbusy = 1;
#ifndef ATM32F43_xx
#define PUT_PUT_DEBUG_BUF_LEN 1024
	static char strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	// vsprintf(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);  //_vsnprintf_s  _vsnprintf
	uint16_t len = vsnprintf(strBuffer, PUT_PUT_DEBUG_BUF_LEN, strOutputString, vlArgs);
	va_end(vlArgs);
	printf(strBuffer);
	OutputDebugStringA(strBuffer); // OutputDebugString    // OutputDebugStringW
	isbusy = 0;
#else
	extern uint8_t USART1_Send_Package(char* string, uint16_t size);
#define PUT_PUT_DEBUG_BUF_LEN 50
	static char strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	// vsprintf(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);  //_vsnprintf_s  _vsnprintf
	uint16_t len = vsnprintf(strBuffer, PUT_PUT_DEBUG_BUF_LEN, strOutputString, vlArgs);
	va_end(vlArgs);
	uint8_t res = 0;
	while (res == 0)
	{
		res = USART1_Send_Package(strBuffer, len); // OutputDebugString    // OutputDebugStringW
		vTaskDelay(1);
	}

#endif
	isbusy = 0;
}

// long ControlThreadCreate(void* function,     // set thread code
// 	void* e,            // set parameter
// 	void* taskhandle_t, // thread instant
// 	const char* name,   // thread name
// 	uint16_t sizeofstack)
// {
// #ifndef ATM32F43_xx
// 	HANDLE hThread;
// 	DWORD dwThreadId;
// 	hThread = CreateThread(NULL,         // 默�?�安全属�?
// 		sizeofstack,  // 默�?�堆栈大�?
// 		function,     // 线程函数
// 		e,            // 传递给线程函数的参�?
// 		0,            // 默�?�创建标�?
// 		&dwThreadId); // 线程ID)

// #else
// 	//	static TaskHandle_t control_thread;
// 	volatile BaseType_t res =
// 		xTaskCreate((TaskFunction_t)function,
// 			(const char*)name,
// 			(uint16_t)sizeofstack,
// 			(void*)e,
// 			(UBaseType_t)5,
// 			(TaskHandle_t*)&taskhandle_t);
// 	return res;
// #endif
// 	return 0;
// }


void thread_create(void* function, void* e, void* control_thread, int sizeofstack,const char* name,int priority)
{
#ifndef ATM32F43_xx
	HANDLE hThread;
	DWORD dwThreadId;
	hThread = CreateThread(NULL,
		0,
		function,
		e,
		0,
		&dwThreadId);
#else
	//	static TaskHandle_t control_thread;
	volatile BaseType_t res =
		xTaskCreate((TaskFunction_t)function,
			(const char*)name,
			(uint16_t)sizeofstack,
			(void*)e,
			(UBaseType_t)priority,
			(TaskHandle_t*)&control_thread);
#endif
}
