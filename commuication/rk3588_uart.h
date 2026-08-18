/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:28
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2024-09-05 11:06:07
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\commuication\rk3588_uart.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __rk3588_uart_H_
#define __rk3588_uart_H_
#include "stdint.h"
#include "stdarg.h"
#ifdef __cplusplus
extern "C"
{
#endif //
	void Rk3588_Init(void);
	int Rk3588_ReadLine(const char* format, ...);
	int Rk3588_ReadLineArg(const char* format, va_list* args);
	int Rk3588_R_ReadLineArg(const char* format, va_list* args);
	int Rk3588_L_ReadLineArg(const char* format, va_list* args);
	void Rk3588_R_Printf(const char* strOutputString, ...);
	void Rk3588_L_Printf(const char* strOutputString, ...);

	int Rk3588_ReadLinearray(char* bytes);
	int Rk3588_R_ReadLinearray(char* bytes);
	int Rk3588_L_ReadLinearray(char* bytes);

#ifdef __cplusplus
}
#endif //

#endif //
