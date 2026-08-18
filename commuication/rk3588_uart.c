/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:28
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2024-09-06 14:19:50
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\commuication\rk3588_uart.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "rk3588_uart.h"
#include "string.h"
#include "stdio.h"
#include <stdarg.h>
#include "../HARDWARE/USART/USART1.h"
#include "../HARDWARE/USART/USART6.h"
#ifdef use_linux
#include "linux_serial.h"
static int rk_right_port = -1;
static int rk_left_port = -1;
#endif

#define buff_size 64
static uint16_t buffIndex, buffIndexL;
static uint8_t buff[buff_size], buffL[buff_size];
static uint8_t newlineFlag, newlineFlagL;
static uint8_t isreading, isreadingL;

void Rk3588_R_Printf(const char* strOutputString, ...)
{
	static uint8_t isbusy = 0;
	//	uint8_t* pt = &isbusy;
		/*while (*pt)
		{
		}
		isbusy = 1;*/
#ifndef ATM32F43_xx
#define PUT_PUT_DEBUG_BUF_LEN 1024
	static char strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	// vsprintf(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);  //_vsnprintf_s  _vsnprintf
	uint16_t len = vsnprintf(strBuffer, PUT_PUT_DEBUG_BUF_LEN, strOutputString, vlArgs);
	va_end(vlArgs);
	printf("%s", strBuffer);
#ifndef use_linux
	OutputDebugStringA(strBuffer); // OutputDebugString    // OutputDebugStringW
#endif
#ifdef use_linux
	LinuxSerial_Write(rk_right_port, (const uint8_t *)strBuffer, len);
#endif
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
		vTaskDelay(10);
	}

#endif
	isbusy = 0;
}

void Rk3588_L_Printf(const char* strOutputString, ...)
{
	static uint8_t isbusy = 0;
	//	uint8_t* pt = &isbusy;
		/*while (*pt)
		{
		}
		isbusy = 1;*/
#ifndef ATM32F43_xx
#define PUT_PUT_DEBUG_BUF_LEN 1024
	static char strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };
	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	// vsprintf(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);  //_vsnprintf_s  _vsnprintf
	uint16_t len = vsnprintf(strBuffer, PUT_PUT_DEBUG_BUF_LEN, strOutputString, vlArgs);
	va_end(vlArgs);
	printf("%s", strBuffer);
#ifndef use_linux
	OutputDebugStringA(strBuffer); // OutputDebugString    // OutputDebugStringW
#endif
#ifdef use_linux
	LinuxSerial_Write(rk_left_port, (const uint8_t *)strBuffer, len);
#endif
	isbusy = 0;
#else
	extern uint8_t USART6_Send_Package(char* string, uint16_t size);
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
		res = USART6_Send_Package(strBuffer, len); // OutputDebugString    // OutputDebugStringW
		vTaskDelay(10);
	}

#endif
	isbusy = 0;
}

int Rk3588_R_ReadLine(const char* format, ...)
{
	if (newlineFlag != 1)
		return -1;
	isreading = 1;
	va_list args;
	va_start(args, format);
	int res = vsscanf(buff, format, args);
	buffIndex = 0;
	newlineFlag = 0;
	va_end(args);
	isreading = 0;
	return res;
}

int Rk3588_R_ReadLineArg(const char* format, va_list* args)
{
	if (newlineFlag != 1)
		return -1;
	isreading = 1;
	int res = vsscanf(buff, format, *args);
	buffIndex = 0;
	newlineFlag = 0;
	isreading = 0;
	return res;
}


int Rk3588_R_ReadLinearray(char* bytes)
{
	if (newlineFlag != 1)
		return -1;
	isreading = 1;
	memcpy(bytes, buff, buffIndex);
	buffIndex = 0;
	newlineFlag = 0;
	isreading = 0;
	return 1;
}

int Rk3588_L_ReadLineArg(const char* format, va_list* args)
{
	if (newlineFlagL != 1)
		return -1;
	isreadingL = 1;
	int res = vsscanf(buffL, format, *args);
	buffIndexL = 0;
	newlineFlagL = 0;
	isreadingL = 0;
	return res;
}

int Rk3588_L_ReadLinearray(char* bytes)
{
	if (newlineFlagL != 1)
		return -1;
	isreadingL = 1;
	memcpy(bytes, buffL, buffIndexL);
	buffIndexL = 0;
	newlineFlagL = 0;
	isreadingL = 0;
	return 1;
}

void Rk3588_R_SerialBuffer(uint8_t byte)
{
	if (isreading)
		return;
	if (buffIndex >= buff_size)
	{
		buffIndex = 0;
	}
	if (byte != '\r')
	{
		buff[buffIndex] = byte;
		buffIndex++;
	}
	if (byte == (uint8_t)'\n')
		newlineFlag = 1;
}

void Rk3588_L_SerialBuffer(uint8_t byte)
{
	if (isreadingL)
		return;
	if (buffIndexL >= buff_size)
	{
		buffIndexL = 0;
	}
	if (byte != '\r')
	{
		buffL[buffIndexL] = byte;
		buffIndexL++;
	}
	if (byte == (uint8_t)'\n')
		newlineFlagL = 1;
}

void Rk3588_Init(void)
{
#ifdef ATM32F43_xx
	USART1_INIT(115200, Rk3588_R_SerialBuffer);
	USART6_INIT(115200, Rk3588_L_SerialBuffer);
#elif defined(use_linux)
	rk_right_port = LinuxSerial_Open("/tmp/ttyRKRight", 115200, Rk3588_R_SerialBuffer);
	rk_left_port = LinuxSerial_Open("/tmp/ttyRKLeft", 115200, Rk3588_L_SerialBuffer);
#else
	SerialPort_Init(Rk3588_R_SerialBuffer, L"COM1");
#endif
}
