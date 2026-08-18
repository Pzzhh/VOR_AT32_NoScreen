/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-09 15:42:15
 * @FilePath: \USERd:\workfile\项目3 vor\software\TC_V2.0\INTERFACE\UI\control\control_TC.c
 * @Description: 这是默�?��?�置,请�?�置`customMade`, 打开koroFileHeader查看配置 进�?��?�置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_ZERO.h"
#include "control_Hardware_API.h"
#include "../other/system_function.h"
#include "control.h"
#include "stdint.h"
#define use_windows
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "../implement/Slave_Zero_ctrl.h"
#endif // use_windows
	   // 无UI控制
// 返回message
// const int singalCount = 1 * 1000;
struct
{
	uint8_t isskip;
} zero_info;

uint8_t HAL_Slave_ZERO_Init(uint8_t isinit)
{
#ifndef ATM32F43_xx
	zero_info.isskip = 1;
#else
	Zero_Machine_Init(isinit);
#endif // !ATM32F43_xx

	return 1;
}

uint8_t HAL_Slave_Get_Flag()
{
#ifndef ATM32F43_xx
	return 1;
#else
	return Zero_Machine_get_flag();
#endif // !ATM32F43_xx
}

uint8_t ZEROControlFunction(uint8_t isinit)
{
	if (zero_info.isskip)
		return 0;
	HAL_Slave_ZERO_Init(isinit);
	uint8_t state=0;
	while (state==0)
	{
		state=HAL_Slave_Get_Flag();
		ControlDelay(100);
	}
	
	return 0;
}
