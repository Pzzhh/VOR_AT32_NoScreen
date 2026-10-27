/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-25 14:38:15
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-08-13 15:27:14
 * @FilePath: \USERd:\workfile\��Ŀ3 vor\software\VOR_V2.0\INTERFACE\UI\control\control_Hardware_API.h
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __control_Hardware_API_H_
#define __control_Hardware_API_H_
#include "stdint.h"
#include "../Task/task_stroage.h"
#include "../other/meassage_center.h"
#ifdef __cplusplus
extern "C"
{
#endif //
#ifndef ATM32F43_xx
#define MYPRINTF(...) printf(__VA_ARGS__)
#else
#define MYPRINTF(...)
#endif

#define Rk3588_Send_Pause                   \
    {                                       \
        e->Rk3588_Flag.Lflag = Pause; \
        e->Rk3588_Flag.Rflag = Pause; \
    }

#define Rk3588_Send_RecordStop                   \
    {                                       \
        e->Rk3588_Flag.Lflag = RecordStop; \
        e->Rk3588_Flag.Rflag = RecordStop; \
    }

#define Rk3588_Send_ReqRkErrorStatus                     \
	{                                               \
		e->Rk3588_Flag.Lflag = ReqRKErrorStatus; \
		e->Rk3588_Flag.Rflag = ReqRKErrorStatus; \
	}

	typedef enum
	{
		Imp_finsih = 0,
		Imp_paused,
		Imp_pausing,
		Imp_running
	} Imp_State;

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
	}timetyped;

	uint8_t HAL_Incline_Init(float angle, uint32_t time);
	uint8_t HAL_Incline_Get_State(float* angle);
	uint8_t HAL_Incline_Stop(void);
	uint8_t HAL_Incline_Fouces_Move(int8_t direction);

	uint8_t HAL_Get_C610_Status(void);
	uint8_t HAL_CAM_Init(void);
	uint8_t HAL_CAM_REC_Set(uint8_t flag);
	uint8_t HAL_CAM_SET_Set(void);
	uint8_t HAL_CAM_SET_sign_led(void);
	uint8_t HAL_CAM_SET_Led_Voltage(uint16_t Millivol);
	uint8_t HAL_Beep_Enable(uint32_t ms);
	uint8_t HAL_IMU_GET_Angle(float* angle);
	uint8_t HAL_Encode_GET_angle(float* angle);
	uint8_t HAL_Encode_GET_angle(float* angle);

	// uint8_t HAL_Control_Get_Start_Cmd(Start_Cmd_Type *cmd);
	uint8_t HAL_Set_UI_Page1_Msg(const char* format, ...);
	uint16_t HAL_Task_GetArray(Task_Parameter_Struct* taskarray, uint16_t arraySize);


	uint8_t HAL_TIME_Set(timetyped* t);
	uint8_t HAL_TIME_Get(timetyped* time);

	int HAL_CONFIG_READ(uint8_t ID, uint8_t* buffer, uint32_t size);
	uint8_t HAL_CONFIG_WRITE(uint8_t ID, uint8_t* buffer, uint32_t size);

	int HAL_TaskStroage_Write(uint8_t id, Task_Parameter_Struct* e, uint8_t count);
	int HAL_TaskStroage_Read(uint8_t id, Task_Parameter_Struct* e);

	int HAL_Outage_Log(uint8_t flag, uint8_t TaskIndex);
	int HAL_Outage_Resume(void);
	int HAL_Outage_Read(uint8_t* buf, uint8_t size);
	int Hal_Outage_time_Read(timetyped* value);
	int Hal_Outage_RunTimeLog(void);

	uint8_t HAL_Get_UI_MouseName(char* str, uint16_t size);
	uint8_t HAL_FAN_Set(uint8_t pwm_precent);
	uint8_t HAL_isBypassVarfiy_key_press();

	void HAL_GET_SYSINFO(char* str);
	uint64_t HAL_GET_SYS_UUID(void);
	uint8_t HAL_Random_Create(void);
	uint8_t HAL_API_INIT(void);

#ifdef __cplusplus
}
#endif //

#endif //
