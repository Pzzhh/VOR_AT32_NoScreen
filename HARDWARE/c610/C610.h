/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-16 09:37:43
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-16 15:33:37
 * @FilePath: \USERd:\workfile\项目11 robomaster\software\01 code\ovor\template\c610\C610.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __C610_H_
#define __C610_H_
#include "stdint.h"
#include "../c610/pid.h"
typedef struct
{
    int16_t current[4];
} C610_Current_Ctrl;

typedef enum
{
    CAN_CHASSIS_ALL_ID = 0x200,
    CAN_3508_M1_ID = 0x201,
    CAN_3508_M2_ID = 0x202,
    CAN_3508_M3_ID = 0x203,
    CAN_3508_M4_ID = 0x204,

    CAN_YAW_MOTOR_ID = 0x205,
    CAN_PIT_MOTOR_ID = 0x206,
    CAN_TRIGGER_MOTOR_ID = 0x207,
    CAN_GIMBAL_ALL_ID = 0x1FF,

} can_msg_id_e;
typedef struct
{
    pid_t V;
    pid_t I;
} C610_Pid;
typedef struct
{
    C610_Pid pid[4];
    int16_t current[4];
} C610_Ctrl;

// rm motor data
typedef struct
{
    uint16_t ecd;
    int16_t speed_rpm;
    int16_t given_current;
    uint8_t temperate;
    int16_t last_ecd;
    int16_t isread;
} motor_measure_t;

/* CAN motor 1 feedback: single-turn encoder angle in degrees and speed in rpm. */
int C610_Get_Feedback(float *angle_deg, float *speed_rpm);

#endif
