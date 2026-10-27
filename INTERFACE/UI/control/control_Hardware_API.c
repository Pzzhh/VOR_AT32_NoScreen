/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-25 14:38:08
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-11-14 16:14:48
 * @FilePath: \USER  d:\workfile\��Ŀ3 vor\software\VOR_V2.0\INTERFACE\UI\control\control_Hardware_API.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "control_Hardware_API.h"
#include "../Task/task_stroage.h"

#include "../other/system_function.h"
#include "../Task/task_info_struct.h"
#include "../other/meassage_center.h"

#include "stdarg.h"
#include "stdio.h"
#include "version.h"

#ifdef ATM32F43_xx

#include "../implement/Slave_Vor_Ctrl.h"
#include "../implement/Slave_Inc_Ctrl.h"
#include "../HARDWARE/CAN/can.h"
#include "../HARDWARE/FAN/FAN.h"
#include "../HARDWARE/RTC/rtc.h"
#include "../HARDWARE/RNG/RNG.h"
#include "../HARDWARE/EROM/eriic.h"
#else
#include <windows.h>
#endif // !ATM32F43_xx

#define use_simluate 0
#if use_simluate

#endif //  use_simluate

#ifdef ATM32F43_xx
#else
timetyped simtime;
#endif
struct
{
	uint32_t time;
	uint32_t SetCount;
	float Setangle;
	float Currentangle;
} inc_info;

#define EROM_MAP_info_base 0x00 // stroage 10 task
#define EROM_MAP_Task_Stroage 0x300

#define EROM_MAP_Task_Stroage_Len 0x1000
#define EROM_MAP_Task_Stroage_pS 0x200 // stroage 10 task
#define EROM_MAP_info_pS 0x20		   // stroage 10 task

struct
{
	struct
	{
		uint8_t online;
	} CAM;
	struct
	{
		uint8_t online;
	} C610;
} CAN_Device;

const uint32_t StroageMaxLength = 12 * sizeof(Task_Parameter_Struct);

uint8_t HAL_Incline_Init(float angle, uint32_t time)
{
	inc_info.Setangle = angle;
#ifdef ATM32F43_xx
	extern uint8_t inc_dir_g;
	return INC_Machine_Init(angle, 0, inc_dir_g);

#endif
	return 1;
}
// 1 running
uint8_t HAL_Incline_Get_State(float *angle)
{
#if use_simluate
	uint32_t tick = ControlGetTick() - inc_info.time;
	if (inc_info.Currentangle >= inc_info.Setangle - 0.1 && inc_info.Currentangle <= inc_info.Setangle + 0.1)
		return 1;
	inc_info.Currentangle += (inc_info.Setangle - inc_info.Currentangle) * 0.05f;
	*angle = inc_info.Currentangle;
	return 0;
#endif
#ifdef ATM32F43_xx
	return INC_Machine_Get_Count(0, angle);
#endif
	return 0;
}

uint8_t HAL_Incline_Stop(void)
{
#ifdef ATM32F43_xx
	INC_Machine_Stop();
#endif
	return 1;
}

uint8_t HAL_Incline_Fouces_Move(int8_t direction)
{
	extern uint8_t inc_dir_g;
#ifdef ATM32F43_xx
	return INC_Machine_Manual_Ctrl(direction, inc_dir_g);
#endif
	return 0;
}

uint8_t HAL_Get_C610_Status(void)
{
#ifdef ATM32F43_xx
	extern int C610_is_Online();
	ControlDelay(100);
	return C610_is_Online();
#else
	return 1;
#endif
}

uint8_t HAL_CAM_Init(void)
{
#ifdef ATM32F43_xx
	// can_init(1, 6, 7, 6, 0); old
	can_init(1, 10, 3, 7, 0);
	extern void C610_Ctrl_Init();
	C610_Ctrl_Init();
	extern void CAM_GPIO_INIT();
	CAM_GPIO_INIT();
	// buf[0] 00  rec
	// buf[0] 01  set
	// buf[0] 02  U16_L U16_H  led voltage Set
	// buf[0] 04  cam Led Flush

#endif
	return 0;
}
// 0 �������� 1 ����
uint8_t HAL_CAM_REC_Set(uint8_t flag)
{
	static uint8_t Cam_Flag = 0;
	if (flag <= 1)
		Cam_Flag = flag;
	else
	{
		Cam_Flag = !Cam_Flag;
	}
	uint8_t res = 1; // reture success
#ifdef ATM32F43_xx
	//	if (Cam_Flag == flag)
	{
		const uint8_t CAN_CAM_MSG[8] = {
			0, 0, 0, 0, 0, 0, 0, 0};
		res = can_send_msg(0x00, (uint8_t *)CAN_CAM_MSG, 8); // can_send reture 1->fail 0->success
		res = (res == 1) ? 0 : 1;							 // exchange reture 1->success 0->fail
		extern void GPIO_CAM_REC_IO_Ctrl(int flag);
		GPIO_CAM_REC_IO_Ctrl(Cam_Flag);
		res = 1;
	}
#endif
	return res;
}

// 0 �������� 1 ����
uint8_t HAL_CAM_SET_Set(void)
{
	uint8_t res = 1; // reture success
#ifdef ATM32F43_xx
	const uint8_t CAN_CAM_MSG[8] = {
		01, 0, 0, 0, 0, 0, 0, 0};
	res = can_send_msg(0x00, (uint8_t *)CAN_CAM_MSG, 8); // can_send reture 1->fail 0->success
	res = (res == 1) ? 0 : 1;							 // exchange reture 1->success 0->fail
	extern void GPIO_CAM_Wifi_IO_Ctrl();
	GPIO_CAM_Wifi_IO_Ctrl();
	res = 1;
#endif
	return res;
}

// 0 �������� 1 ����
uint8_t HAL_CAM_SET_sign_led(void)
{
	uint8_t res = 1; // reture success
#ifdef ATM32F43_xx
	const uint8_t CAN_CAM_MSG[8] = {
		04, 0, 0, 0, 0, 0, 0, 0};
	res = can_send_msg(0x00, (uint8_t *)CAN_CAM_MSG, 8); // can_send reture 1->fail 0->success
	res = (res == 1) ? 0 : 1;							 // exchange reture 1->success 0->fail
	extern void GPIO_CAM_SyncLED_IO_Ctrl();
	GPIO_CAM_SyncLED_IO_Ctrl();
#endif
	return res;
}

// 0 error 1 normal
uint8_t HAL_CAM_SET_Led_Voltage(uint16_t Millivol)
{
	uint8_t res = 1; // reture success
	uint8_t *vol = (uint8_t *)&Millivol;
#ifdef ATM32F43_xx
	const uint8_t CAN_CAM_MSG[8] = {
		02, vol[0], vol[1], 0, 0, 0, 0, 0};
	uint8_t wait = 10;
	while (wait--)
	{
		res = can_send_msg(0x00, (uint8_t *)CAN_CAM_MSG, 8); // can_send reture 1->fail 0->success
															 /* code */
		if (res == 1)
			break;
		delay_ms(1);
	}

	//res = (res == 1) ? 0 : 1; // exchange reture 1->success 0->fail
	res = wait;
#endif
	return res;
}

// 0 error 1 normal
uint8_t HAL_Beep_Enable(uint32_t ms)
{

#ifdef ATM32F43_xx
	GPIO_Beep_IO_Ctrl(ms); // exchange reture 1->success 0->fail
#endif
	return 1;
}

uint8_t HAL_IMU_GET_Angle(float *angle)
{
#ifdef ATM32F43_xx
	extern float JY60_Get_IncAsync();
	if (angle != 0)
	{
		*angle = JY60_Get_IncAsync();
	}
#else
	if (angle != 0)
	{
		*angle = 1.335f;
	}
#endif

	return 1;
}

uint8_t HAL_Encode_GET_angle(float *angle)
{
#ifdef ATM32F43_xx
	extern float Slave1_Get_Encode_Angle(void);
	if (angle != 0)
	{
		*angle = Slave1_Get_Encode_Angle();
	}
#else
	static float anglex;
	if (angle != 0)
	{
		anglex += 5.0f;
		*angle = anglex;
	}
#endif

	return 1;
}

void HAL_GET_SYSINFO(char *str)
{
#ifdef ATM32F43_xx
	uint32_t sn[3];
	// Unique ID is stored in 0x1FFF7A10, 0x1FFF7A14, and 0x1FFF7A18
	sn[0] = *(uint32_t *)0x1FFF7A10;
	sn[1] = *(uint32_t *)0x1FFF7A14;
	sn[2] = *(uint32_t *)0x1FFF7A18;
	uint8_t isDebuggerConnected = *((uint32_t *)0xE000EDF0) & 1;
	if (isDebuggerConnected)
		sprintf(str, "V2.%d.%d Debug\tSN:%.8x", Version_NUMBER_Major, Version_NUMBERB, sn[1]);
	else
		sprintf(str, "V2.%d.%d Beta \tSN:%.8x", Version_NUMBER_Major, Version_NUMBERB, sn[1]);
	return;
#endif
	sprintf(str, "V2.1.%d \tSN:%.8x", Version_NUMBER_Major, 0x550);
	return;
}

uint64_t HAL_GET_SYS_UUID(void)
{
#ifdef ATM32F43_xx
	uint32_t sn[3];
	// Unique ID is stored in 0x1FFF7A10, 0x1FFF7A14, and 0x1FFF7A18
	sn[0] = *(uint32_t *)0x1FFF7A10;
	sn[1] = *(uint32_t *)0x1FFF7A14;
	sn[2] = *(uint32_t *)0x1FFF7A18;
	uint64_t *uuid = (uint64_t *)sn;
	return *uuid;
#endif

	return 0x550;
}
#include "math.h"
uint8_t HAL_Random_Create(void)
{
	unsigned int send = 0;
#ifdef ATM32F43_xx
	return RNG_Generate();
#endif
	return send;
}

uint8_t HAL_Random_INIT(void)
{
#ifdef ATM32F43_xx
	RNG_Generate_Init();
#endif
}

// void HAL_RTC_SET()
// {

// }

// void HAL_RTC_

// void Hal_RTC_INIT()
// {

// }

uint8_t HAL_Set_UI_Page1_Msg(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	extern char *UI_Page1_Get_Msg_Array(uint16_t *len);
	extern void UI_Page1_Set_Msg_Finish(void);
	char *msg_pt = UI_Page1_Get_Msg_Array(0);
	while (msg_pt == 0)
	{
		msg_pt = UI_Page1_Get_Msg_Array(0);
	}
	vsprintf(msg_pt, format, args);
	va_end(args);
	UI_Page1_Set_Msg_Finish();
	return 1;
}

uint8_t HAL_Get_UI_MouseName(char *str, uint16_t size)
{
	return (uint8_t)Message_Center_Read_prinft("page1", str, size, "MouseName?");
}

uint16_t HAL_Task_GetArray(Task_Parameter_Struct *taskarray, uint16_t arraySize)
{
	uint16_t res = Task_Stroage_GetArray(taskarray, arraySize);
	return res;
}

uint8_t HAL_FAN_INIT()
{
#ifdef ATM32F43_xx
	FAN_Init();
#endif
	return 1;
}

uint8_t HAL_FAN_Set(uint8_t pwm_precent)
{
#ifdef ATM32F43_xx
	FAN_Ctrl(pwm_precent);
#endif
	return 1;
}

uint8_t HAL_is_Vaild_Expired()
{
	timetyped time;
	uint8_t res = HAL_CONFIG_READ(1, (uint8_t *)&time, sizeof(time));
	return res;
}

uint8_t HAL_Set_Vaild_Date(timetyped *time)
{
	return 0;
}

uint8_t HAL_isBypassVarfiy_key_press()
{
	uint8_t keypress = 0;
#ifdef ATM32F43_xx

#endif
	return keypress;
}

uint8_t HAL_TIME_Set(timetyped *t)
{
#ifdef ATM32F43_xx
	uint8_t timearray[4] = {0};
	rtc_set_date(t->date.year, t->date.month, t->date.date, 1);
	rtc_set_time(t->date.hour, t->date.min, 0, 0);
#else
	simtime = *t;
#endif
	return 1;
}

uint8_t HAL_TIME_Init()
{
#ifdef ATM32F43_xx
	return rtc_init();
#endif
	return 1;
}

uint8_t HAL_TIME_Get(timetyped *time)
{
#ifdef ATM32F43_xx
	uint8_t timearray[4] = {0};
	rtc_get_time(timearray, timearray + 1,
				 timearray + 2, timearray + 3);
	if (timearray[3])
	{
		timearray[0] += 12;
	}
	time->date.hour = timearray[0];
	time->date.min = timearray[1];
	time->date.sec = timearray[2];
	rtc_get_date(timearray, timearray + 1,
				 timearray + 2, timearray + 3);
	time->date.year = timearray[0];
	time->date.year += 2000;
	time->date.month = timearray[1];
	time->date.date = timearray[2];
#else
	SYSTEMTIME st;
	GetLocalTime(&st);
	simtime.date.year = st.wYear;
	simtime.date.month = st.wMonth;
	simtime.date.date = st.wDay;
	simtime.date.hour = st.wHour;
	simtime.date.min = st.wMinute;
	simtime.date.sec = st.wSecond;
	*time = simtime;
#endif
	return 1;
}
#ifndef ATM32F43_xx
#include "direct.h"

#endif

uint16_t crc16_modbus(const uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFF; // ��ʼֵ
	for (uint16_t i = 0; i < length; i++)
	{
		crc ^= data[i]; // ���ֽ����
		for (uint8_t j = 0; j < 8; j++)
		{
			if (crc & 0x0001)
			{
				crc = (crc >> 1) ^ 0xA001; // ���Ʋ�������ʽ 0xA001
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}

uint16_t crc16_Custom(const uint8_t *data, uint16_t length, uint16_t crcfactor)
{
	uint16_t crc = 0xFFFF; // ��ʼֵ
	for (uint16_t i = 0; i < length; i++)
	{
		crc ^= data[i]; // ���ֽ����
		for (uint8_t j = 0; j < 8; j++)
		{
			if (crc & 0x0001)
			{
				crc = (crc >> 1) ^ crcfactor; // ���Ʋ�������ʽ 0xA001
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}

int HAL_TaskStroage_Read(uint8_t id, Task_Parameter_Struct *e)
{
	if (id >= 4)
	{
		return 0;
	}
#ifndef ATM32F43_xx
	int res = 0, i = 0;
	char Path[1000];
	uint8_t Rbuf[0x500], *RbufPt;

	// e = TaskArray;
	if (_getcwd(Path, 1000) == NULL)
		return 0;
	int pos = (int)strlen(Path);
#ifdef use_linux
	sprintf(&Path[pos], "/Task.hex");
#else
	sprintf(&Path[pos], "\\Task.hex");
#endif
	FILE *f = fopen(Path, "r");
	if (f == NULL)
	{
		f = fopen(Path, "w");
		if (f == NULL)
			return res;
	}
	int readcount = fread(Rbuf, 0x500, 1, f);
	fclose(f);
	uint16_t datalen = 0;
	if (readcount != 0)
	{
		RbufPt = Rbuf + id * StroageMaxLength;
		uint16_t *pr = (uint16_t *)RbufPt;
		datalen = pr[1];
		if (datalen > 0x500)
			return 0;
		uint16_t Crc = crc16_modbus(&RbufPt[6], datalen);
		uint16_t DataCrc = pr[0];
		uint16_t structSize = sizeof(Task_Parameter_Struct);
		uint16_t DatastructSize = pr[2];
		if (Crc == DataCrc && structSize == DatastructSize)
		{
			memcpy(e, &RbufPt[6], datalen);
			if (structSize == 0)
				return 0;
			res = datalen / structSize;
		}
	}

	// crc check
	return res;
#else

	// volatile const uint8_t* p = (volatile const uint8_t*)0x40024500 + id * 12 * sizeof(Task_Parameter_Struct);
	static volatile uint8_t p[EROM_MAP_Task_Stroage_pS] = {0};
	M24C64_Read_Byte(id * EROM_MAP_Task_Stroage_pS + EROM_MAP_Task_Stroage,
					 (uint8_t *)p, EROM_MAP_Task_Stroage_pS);
	uint16_t storedCRC = (uint16_t)(p[0] | (p[1] << 8));
	uint16_t totalLen = (uint16_t)(p[2] | (p[3] << 8));
	uint16_t structSize = (uint16_t)(p[4] | (p[5] << 8));
	const uint8_t *data = (const uint8_t *)(p + 6);
	if (totalLen > StroageMaxLength)
		return 0;
	uint16_t calc = crc16_modbus(data, totalLen);
	if (calc != storedCRC)
		return 0;
	if (structSize != sizeof(Task_Parameter_Struct))
		return 0;
	if ((totalLen % structSize) != 0)
		return 0;

	memcpy(e, data, totalLen);
	return (int)(totalLen / structSize);
	// return 0;
#endif
}

int HAL_TaskStroage_Write(uint8_t id, Task_Parameter_Struct *e, uint8_t count)
{
	if (id >= 4)
	{
		return 0;
	}
#ifndef ATM32F43_xx
	char Path[1000];
	uint8_t buf[0x500] = {0}, *wrpt;
	if (count * sizeof(Task_Parameter_Struct) > 480)
		return 0;
	_getcwd(Path, 1000);
	int pos = strlen(Path);
#ifdef use_linux
	sprintf(&Path[pos], "/Task.hex");
#else
	sprintf(&Path[pos], "\\Task.hex");
#endif
	FILE *f = fopen(Path, "r+");
	if (f == NULL)
	{
		f = fopen(Path, "w");
		if (f == NULL)
			return 0;
	}
	fread(buf, sizeof(buf), 1, f);
	fclose(f);
	f = fopen(Path, "w+");
	if (f == 0)
		return 0;
	int structsize = sizeof(Task_Parameter_Struct) * count;
	uint16_t Crc = crc16_modbus(e, structsize);
	uint16_t prefix[] = {Crc, structsize, sizeof(Task_Parameter_Struct)};
	wrpt = buf + id * 12 * sizeof(Task_Parameter_Struct);
	memcpy(wrpt, prefix, 6);
	memcpy(&wrpt[6], e, structsize);
	fwrite(buf, sizeof(buf), 1, f);
	fclose(f);
	return 1;
#else
#define TASK_BKPSRAM_SIZE (4096U)
#define TASK_BKP_MAX_DATA (480U)  /* �� PC �˱���һ�£��ɰ������ */
#define TASK_BKP_HEADER_SIZE (6U) /* CRC(2)+TotalLen(2)+StructSize(2) */
#define TASK_BKP_MAGIC (0xA55A)
	uint32_t totalLen = (uint32_t)count * sizeof(Task_Parameter_Struct);
	if (totalLen == 0 || totalLen > TASK_BKP_MAX_DATA)
		return 0;
	if (TASK_BKP_HEADER_SIZE + totalLen > TASK_BKPSRAM_SIZE)
		return 0;

	uint16_t crc = crc16_modbus((const uint8_t *)e, totalLen);

	static uint8_t p[EROM_MAP_Task_Stroage_pS] = {0};

	// uint8_t *p = (uint8_t *)0x40024500 + id * 12 * sizeof(Task_Parameter_Struct);

	/* ֱ��д�� (Backup SRAM ������ͨ SRAM һ��д) */
	p[0] = (uint8_t)(crc & 0xFF);
	p[1] = (uint8_t)(crc >> 8);
	p[2] = (uint8_t)(totalLen & 0xFF);
	p[3] = (uint8_t)(totalLen >> 8);
	uint16_t structSize = (uint16_t)sizeof(Task_Parameter_Struct);
	p[4] = (uint8_t)(structSize & 0xFF);
	p[5] = (uint8_t)(structSize >> 8);

	memcpy(&p[TASK_BKP_HEADER_SIZE], e, totalLen);

	M24C64_Write_Byte(id * EROM_MAP_Task_Stroage_pS + EROM_MAP_Task_Stroage,
					  (uint8_t *)p, EROM_MAP_Task_Stroage_pS);
	M24C64_Read_Byte(id * EROM_MAP_Task_Stroage_pS + EROM_MAP_Task_Stroage,
					 (uint8_t *)p, EROM_MAP_Task_Stroage_pS);
	/* ��ѡ����У��һ��д����ȷ */
	uint16_t verify = crc16_modbus(&p[TASK_BKP_HEADER_SIZE], (uint16_t)totalLen);
	return (verify == crc) ? 1 : 0;
	return 0;
#endif
}

int HAL_Outage_Log(uint8_t flag, uint8_t TaskIndex)
{
	return 0;
	uint8_t value[2] = {0};
	if (flag)
	{
		value[0] = 1;
		value[1] = TaskIndex;
	}
	HAL_CONFIG_WRITE(4, value, sizeof(value));
	if (value[0] == 1)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "task save %d", 3);
		Hal_Outage_RunTimeLog();
	}
	return 0;
}

int HAL_Outage_Resume()
{
	uint8_t value[2] = {0};
	HAL_CONFIG_READ(4, (uint8_t *)&value, sizeof(value));
	if (value[0] == 1)
	{
		Message_Center_Read_prinft("Ctrl", 0, 0,
								   "task load %d", 3);
	}
	return 0;
}

int HAL_Outage_Read(uint8_t *buf, uint8_t size)
{
	uint8_t value[2] = {0};
	if (size != sizeof(value))
		return 0;
	HAL_CONFIG_READ(4, (uint8_t *)&value, sizeof(value));
	memcpy(buf, value, size);
	return 1;
}

int Hal_Outage_time_Read(timetyped *value)
{
	int res = HAL_CONFIG_READ(5, (uint8_t *)value, sizeof(timetyped));
	return res;
}

int Hal_Outage_RunTimeLog()
{
	timetyped value;
	HAL_TIME_Get(&value);
	int res = HAL_CONFIG_WRITE(5, (uint8_t *)&value, sizeof(timetyped));
	return res;
}

int HAL_CONFIG_READ(uint8_t ID, uint8_t *buffer, uint32_t size)
{
#ifndef ATM32F43_xx
	int res = 0;
	char Path[1000];
	uint8_t Rbuf[500];
	_getcwd(Path, 1000);
	int pos = strlen(Path);
#ifdef use_linux
	sprintf(&Path[pos], "/config.hex");
#else
	sprintf(&Path[pos], "\\config.hex");
#endif
	FILE *f = fopen(Path, "r");
	if (f == NULL)
	{
		f = fopen(Path, "w");
		if (f == NULL)
			return res;
	}
	int readcount = fread(Rbuf, 500, 1, f);
	fclose(f);
	if (readcount != 0)
	{
		int count = 0;
		for (int i = 0; i < 500; i += 50)
		{
			if (Rbuf[i] == ID)
			{
				uint32_t crc_Len = Rbuf[1 + i];
				uint16_t Crc = crc16_modbus(&Rbuf[i + 2], crc_Len);
				uint16_t Filecrc = Rbuf[i + crc_Len + 2];
				Filecrc = Filecrc << 8;
				Filecrc = Filecrc + Rbuf[i + crc_Len + 3];
				if (Filecrc == Crc)
				{
					if (size >= crc_Len)
					{
						memcpy(buffer, &Rbuf[i + 2], crc_Len);
						res = crc_Len;
					}
					else
					{
						res = 0;
					}
				}
			}
			count++;
		}
	}

	// crc check
	return res;
#else
	if (ID >= (EROM_MAP_Task_Stroage - EROM_MAP_info_base) / EROM_MAP_info_pS)
		return 0;
	int res = 0;
	uint16_t crcValue;
	// uint8_t *BKSbuf = (uint8_t *)0x40024000 + ID * 20;
	uint8_t *BKSbuf = (uint8_t *)malloc(EROM_MAP_info_pS);
	if (BKSbuf == 0)
		return 0;
	M24C64_Read_Byte(ID * EROM_MAP_info_pS + EROM_MAP_info_base, BKSbuf, EROM_MAP_info_pS);
	int readcount = BKSbuf[1];
	uint16_t ReadEnd = readcount + 2;
	// data format:
	// first id len [...] crc
	if (BKSbuf[0] == ID)
	{
		if (readcount < 50)
		{
			crcValue = crc16_modbus(BKSbuf, readcount + 2);
			uint16_t Ocrc = BKSbuf[ReadEnd];
			Ocrc = Ocrc * 256 + BKSbuf[ReadEnd + 1];
			if (crcValue == Ocrc)
			{
				res = readcount;
				memcpy(buffer, &BKSbuf[2], readcount);
			}
		}
	}
	free(BKSbuf);
	return res;
#endif
}

uint8_t HAL_CONFIG_WRITE(uint8_t ID, uint8_t *buffer, uint32_t size)
{
#ifndef ATM32F43_xx
	char Path[1000];
	uint8_t buf[500] = {0};
	if (size > 46)
		return 0;
	_getcwd(Path, 1000);
	int pos = strlen(Path);
#ifdef use_linux
	sprintf(&Path[pos], "/config.hex");
#else
	sprintf(&Path[pos], "\\config.hex");
#endif
	FILE *f = fopen(Path, "r+");
	if (f == 0)
		return 0;
	fread(buf, sizeof(buf), 1, f);
	fclose(f);
	f = fopen(Path, "w+");
	if (f == 0)
		return 0;
	int count = 0;
	for (int i = 0; i < 500; i += 50)
	{
		if (count == ID)
		{
			uint16_t Crc = crc16_modbus(buffer, size);
			uint8_t crcHL[2] = {Crc >> 8, Crc & 0xff};
			buf[i] = ID;
			buf[i + 1] = size;
			memcpy(&buf[i + 2], buffer, size);
			memcpy(&buf[i + size + 2], crcHL, 2);
		}
		count++;
	}

	fwrite(buf, sizeof(buf), 1, f);
	fclose(f);
	return 1;
#else
	int res = 0;
	uint16_t crcValue;
	uint8_t *buf = (uint8_t *)malloc(EROM_MAP_info_pS);
	if (buf == 0)
		return 0;
	// uint8_t* buf = (uint8_t*)0x40024000 + ID * 20;
	//	int readcount = buf[1];
	int buf_end;
	// data format:
	// first id len [...] crc
	if (size < EROM_MAP_info_pS - 2)
	{
		if (size == 0)
		{
			memset(&buf[0], 0,EROM_MAP_info_pS);
			M24C64_Write_Byte(ID * EROM_MAP_info_pS + EROM_MAP_info_base, buf, EROM_MAP_info_pS);
			goto out;
		}
		memcpy(&buf[2], buffer, size);
		buf[0] = ID;
		buf[1] = size;
		crcValue = crc16_modbus(buf, size + 2);
		buf_end = size + 2;
		buf[buf_end] = crcValue >> 8;
		buf[buf_end + 1] = crcValue & 0xff;
		M24C64_Write_Byte(ID * EROM_MAP_info_pS + EROM_MAP_info_base, buf, EROM_MAP_info_pS);
		res = 1;
	}
	out:
	free(buf);
	return res;
#endif
}

void HAL_EROM_Init(void)
{
#ifdef ATM32F43_xx
	ER_IIC_Init();
#endif
}

void HAL_Motor_init(void)
{
#ifdef ATM32F43_xx
	motor_init();
#endif
}

uint8_t HAL_API_INIT(void)
{
	HAL_Motor_init();
	HAL_CAM_Init();
	HAL_CAM_SET_Set();
	HAL_FAN_INIT();
	HAL_EROM_Init();
	uint8_t res = HAL_TIME_Init();
	if (res == 1) // loss power
	{
		uint8_t buf[32] = {0};
		HAL_CONFIG_WRITE(3, buf, sizeof(buf) - 3);
	}
	HAL_Random_INIT();
#ifdef ATM32F43_xx
	RNG_Generate();
#endif
	return 0;
}
