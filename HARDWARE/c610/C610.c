/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-16 09:37:35
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-11-20 14:53:31
 * @FilePath: \USERd:\workfile\项目11 robomaster\software\01 code\ovor\template\c610\C610.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE10
 */
#include "C610.h"
#include "../c610/pid.h"
#include "../CAN/can.h"
#include "at32f435_437.h"

motor_measure_t motor_chassis[7];
extern struct data1 State;

#define get_motor_measure(ptr, data)                                   \
    {                                                                  \
        (ptr)->last_ecd = (ptr)->ecd;                                  \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);           \
        (ptr)->speed_rpm = (uint16_t)((data)[2] << 8 | (data)[3]);     \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]); \
        (ptr)->temperate = (data)[6];                                  \
    }

float SetSpeed = 0, SetCurrent;
// float TargetCurrent, MotorPower;
pid_t Vpid, Ipid;
motor_measure_t *mymotor;
C610_Ctrl rotate;
static uint8_t isC610Online = 0;

void C610_Current_Cmd(C610_Current_Ctrl buf)
{
    can_send_msg(0x200, (uint8_t *)&buf, 8);
    // can_tx_msg(0x200, 0, 0, 8, (uint8_t *)&buf);
}

void C610_Updata_Info(int16_t t_rpm, int16_t m_rpm, int16_t m_current, int16_t m_pwm)
{
    uint16_t buf[4] = {t_rpm, m_rpm, m_current, m_pwm};
    can_send_msg(0x500, (uint8_t *)&buf, 8);
}

void C610_Current_Cmd2(int16_t a, int16_t b, int16_t c, int16_t d)
{
    uint8_t buf[8];
    buf[0] = a >> 8;
    buf[1] = a & 0x00ff;
    buf[2] = b >> 8;
    buf[3] = b & 0x00ff;
    buf[4] = c >> 8;
    buf[5] = c & 0x00ff;
    buf[6] = d >> 8;
    buf[7] = d & 0x00ff;
    can_tx_msg(0x200, 0, 0, 8, (uint8_t *)&buf);
}

void Motor_Spd_Pid_INIT()
{
    int ch = 1;
    for (int i = 0; i < 4; i++)
    {
        PID_struct_init(&rotate.pid[i].V, POSITION_PID, 1500, 500, 20, 0.2f, 5.0f);    // 速度
        PID_struct_init(&rotate.pid[i].I, POSITION_PID, 1500, 500, 1.0f, 0.05f, 0.0f); // 电流环
        /* code */
    }
}

int16_t C610_PID_CAL(C610_Pid *PID, motor_measure_t *m, float Speed)
{
    float TargetCurrent, MotorPower;
    if (Speed <= 1 && Speed >= -1)
        return 0;
    TargetCurrent = pid_calc(&PID->V, m->speed_rpm, Speed);
    MotorPower = pid_calc(&PID->I, m->given_current, TargetCurrent);
    return (int16_t)MotorPower;
}
// float factor = -65;
float C610_Set_Speed = 0;
void Motor_Set_Speed(float speed)
{
    const float factor = -1;
    C610_Set_Speed = factor * speed;
}

void Motor_Spd_Pid(void)
{
    if (isC610Online == 0)
        return;
    if (C610_Set_Speed == 0)
    {
        C610_Current_Cmd2(0, 0, 0, 0);
        return;
    }
    int16_t pwm;
    float C610_Speed = -(C610_Set_Speed) * (60.0f * 36.0f) / (-65);
    pwm = C610_PID_CAL(&rotate.pid[0], &motor_chassis[0], C610_Speed);
    C610_Current_Cmd2(pwm, 0, 0, 0);
    C610_Updata_Info(C610_Speed,
                     motor_chassis[0].speed_rpm,
                     motor_chassis[0].given_current,
                     pwm);
}

float Avarage(float *val, int len)
{
    float res = 0;
    for (int i = 0; i < len; i++)
    {
        res += val[i];
    }
    res /= 10;
    return res;
}

int C610_is_Online()
{
    return isC610Online;
}

int C610_Get_Feedback(float *angle_deg, float *speed_rpm)
{
    uint16_t ecd;
    int16_t rpm;

    if (!isC610Online || angle_deg == 0 || speed_rpm == 0)
        return 0;
    ecd = motor_chassis[0].ecd;
    rpm = motor_chassis[0].speed_rpm;
    *angle_deg = (float)ecd * (360.0f / 8192.0f);
    *speed_rpm = (float)rpm;
    return 1;
}

void C610_RX_handler(uint32_t id, uint8_t *msg)
{
    static float Vfilter[4][10];
    static float Ifilter[4][10];
    static int index;
    switch (id)
    {
    case CAN_3508_M1_ID:
    case CAN_3508_M2_ID:
    case CAN_3508_M3_ID:
    case CAN_3508_M4_ID:
    case CAN_YAW_MOTOR_ID:
    case CAN_PIT_MOTOR_ID:
    case CAN_TRIGGER_MOTOR_ID:
    {
        static uint8_t i = 0;
        isC610Online = 1;
        // get motor id
        i = id - CAN_3508_M1_ID;
        if (index >= 10)
            index = 0;
        if (i < 4)
        {
            get_motor_measure(&motor_chassis[i], msg);
            Vfilter[i][index] = motor_chassis[i].speed_rpm;
            Ifilter[i][index] = motor_chassis[i].given_current;
            index++;
            motor_chassis[i].speed_rpm = Avarage(Vfilter[i], 10);
            motor_chassis[i].given_current = Avarage(Ifilter[i], 10);
        }
        break;
    }

    default:
    {
        break;
    }
    }
}

/**
 * @brief       CAN RX0 中断服务函数
 *   @note      处理CAN FIFO0的接收中断
 * @param       无
 * @retval      无
 */
void C610_CAN_RX_HANDLER(can_rx_message_type *msg)
{
    // uint8_t rxbuf[8];
    // uint32_t id;
    // uint8_t ide, rtr, len;
    // can_rx_msg(0, &id, &ide, &rtr, &len, rxbuf);
    uint32_t id = msg->standard_id;
    uint8_t *rxbuf = msg->data;
    C610_RX_handler(id, rxbuf);
}

void C610_Ctrl_Init()
{
    can_rx_handler = C610_CAN_RX_HANDLER;
    Motor_Spd_Pid_INIT();
}
