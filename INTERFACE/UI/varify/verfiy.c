#include "control.h"
#include "../control/control_Hardware_API.h"
#include "../../commuication/communication_API.h"
#include "../other/system_function.h"
#include "stdint.h"
#include "../other/meassage_center.h"
#include "stdarg.h"

#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#endif // use_windows
#include "verfiy.h"
#include "../control/control_Hardware_API.h"
#include "../control/control.h"
#define Using_UI_LOCK 1

#define use_windows
#ifdef ATM32F43_xx
#else
typedef uint32_t TaskHandle_t;

#endif
timetyped InfiniteTag = { .date.year = 99,
						.date.month = 1,
.date.date = 1,
.date.hour = 1,
.date.min = 1,
.date.sec = 1 };


const timetyped HardResetvaildtag = { .date.year = 2025,
					.date.month = 11,
					.date.date = 18,
					.date.hour = 1,
					.date.min = 1,
					.date.sec = 1 };
//timetyped LocalTimeTag = { .date.year = 26,
//						.date.month = 1,
//.date.date = 1,
//.date.hour = 1,
//.date.min = 1,
//.date.sec = 1 };

uint8_t Verify_Compare_time(timetyped* expiredTag, timetyped localtime)
{
	uint8_t status = 0;
	for (int i = 0; i < 3; i++)
	{
		if (localtime.num[i] >= expiredTag->num[i])
		{
			status = expired;
		}
		else
		{
			status = okay;
			break;
		}
	}
	return status;
}


// 判断是否为闰年
static int is_leap_year(int year) {
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 获取某年某月的天数
static int get_month_days(int year, int month) {
	static const int days[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (month == 2 && is_leap_year(year)) {
		return 29;
	}
	return days[month - 1];
}

// 将日期转换为自某一基准日以来的总天数
static int date_to_days(const timetyped* t) {
	int y = t->date.year;
	int m = t->date.month;
	int d = t->date.date;
	int days = 0;

	// 计算年份的天数
	for (int i = 0; i < y; ++i) {
		days += is_leap_year(i) ? 366 : 365;
	}
	// 计算月份的天数
	for (int i = 1; i < m; ++i) {
		days += get_month_days(y, i);
	}
	// 加上日期
	days += d;
	return days;
}

void  Verify_TimeCheck(timetyped* time)
{
	uint8_t rflag = 0;
	uint8_t res = 1;
	const int dayN[] = { 0,31,29,31,30,31,30,
						31,31,30,31,30,31 };
	const int dayR[] = { 0,31,28,31,30,31,30,
						31,31,30,31,30,31 };
	if (time->date.year % 4 == 0)
		rflag = 1;
	const int* day = (rflag) ? dayR : dayN;
	time->date.month = (time->date.month > 12 || time->date.month == 0) ? 12 : time->date.month;
	time->date.date = (time->date.date > day[time->date.month]) ? day[time->date.month] : time->date.date;

}

// 计算 expiredTag 距离 localtime 剩余天数（正数为剩余，负数为已过期）
int Verify_Calc_RemainDays(const timetyped* expiredTag, const timetyped* localtime)
{
	int days_expired = date_to_days(expiredTag);
	int days_local = date_to_days(localtime);
	return days_expired - days_local;
}

void Verify_hardResetVaild(void)
{

	timetyped localtime = { 0 };
	HAL_TIME_Get(&localtime);
	uint8_t buf[50] = { 0 };
	uint8_t res = HAL_CONFIG_READ(1, buf, 0x20);
	// 自动设置 设备有效期

	if (res != 0)
	{
		timetyped* expiredTag = (timetyped*)&buf[0];
		if (Verify_Calc_RemainDays(&HardResetvaildtag, &localtime) > 0)
		{
			if (memcmp(expiredTag, &HardResetvaildtag, sizeof(HardResetvaildtag)) != 0)
			{
				memcpy(expiredTag, &HardResetvaildtag, sizeof(HardResetvaildtag));
				Verify_TimeCheck(expiredTag);
				HAL_CONFIG_WRITE(1, (uint8_t*)expiredTag, sizeof(timetyped));
			}
		}
	}
}

uint8_t Verify_Check_isExpired(void)
{
	uint8_t buf[50] = { 0 };
	uint8_t status = NoTag;
	//check expired time tag detail
		//read time using day
	//HAL_CONFIG_WRITE(1, &testTag, sizeof(testTag));

	//if (HAL_isBypassVarfiy_key_press())
	//{
	//	status = SetTimeOnly;
	//	return status;
	//}
	//else
	{
		uint8_t res = HAL_CONFIG_READ(3, buf, 0x20);
		if (res == 0)
		{
			status = SeTboth;
			return status;
		}
	}
	uint8_t res = HAL_CONFIG_READ(1, buf, 0x20);

	// 自动设置 设备有效期

	if (res != 0)
	{
		timetyped* expiredTag = (timetyped*)&buf[0];
		timetyped localtime;
		HAL_TIME_Get(&localtime);
		Message_Center_Read_prinft("Ctrl", &localtime, sizeof(localtime), "ReqTime");
		status = Verify_Compare_time(expiredTag, localtime);
	}
	else
	{
		/*HAL_CONFIG_WRITE(1, &InfiniteTag, sizeof(InfiniteTag));*/
	}
	return status;
}


//0 infinite
uint8_t Verify_get_license_Date(timetyped* t)
{

	uint8_t buf[0x20] = { 0 };
	uint8_t res = HAL_CONFIG_READ(1, buf, 0x20);
	if (res != 0)
	{
		memcpy(t, buf, sizeof(timetyped));
		return 1;
	}
	return 0;
}




timetyped Verify_Month_2_Expired_Date(uint8_t month)
{
	timetyped localtime = { 0 };
	HAL_TIME_Get(&localtime);
	uint8_t year = month / 12;
	uint8_t remain_month = month % 12;
	uint8_t needNextyear=((12-localtime.date.month)<remain_month)?1:0;
	if(needNextyear)
		year++;
	localtime.date.year += year;
	localtime.date.month += remain_month;
	localtime.date.month%=12;
	Verify_TimeCheck(&localtime);
	HAL_CONFIG_WRITE(1, (uint8_t*)&localtime, sizeof(localtime));
}

uint8_t Verify_Set_Time(void* time)
{
	uint8_t buf[2] = { 0xAA,0xAA }, status = 0;
	HAL_TIME_Set(time);
	HAL_CONFIG_WRITE(3, buf, 2);
}


uint8_t Verify_Factory_Reset(void)
{
	uint8_t buf[0x20] = { 0 };
	HAL_CONFIG_WRITE(1, (uint8_t*)buf, 0);
	HAL_CONFIG_WRITE(3, (uint8_t*)buf, 0);
}

uint8_t Verify_Set_Subscribe(uint8_t month)
{

	if (month == 0xFF)
	{
		HAL_CONFIG_WRITE(1, (uint8_t*)&InfiniteTag, sizeof(InfiniteTag));
		return 0;
	}
	Verify_Month_2_Expired_Date(month);
	return 0;
}



