/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-27 18:24:37
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\UI_Include.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __UI_include_H_
#define __UI_include_H_

#ifdef ATM32F43_xx
#include "LVGL/lvgl.h"
#include "lv_conf.h"
#include "stdint.h"
typedef struct {
		struct
		{
			uint16_t year;
			uint16_t month;
			uint16_t date;
			uint16_t hour;
			uint16_t min;
			uint16_t sec;
		}date;
}timetyped_UI;
#else

#include "lvgl/lvgl.h"
#include "lv_conf.h"
typedef struct {
	union {
		uint16_t num[6];
		struct
		{
			uint16_t year;
			uint16_t month;
			uint16_t date;
			uint16_t hour;
			uint16_t min;
			uint16_t sec;
		}date;
	};
}timetyped_UI;

#endif // 


#endif // !__UI_include_H_
