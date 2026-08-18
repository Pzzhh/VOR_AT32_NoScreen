/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-27 13:50:34
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_Inc_Ctrl.h"
#include "math.h"
#include "delay.h"
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"
#include "../HARDWARE/INC/INC_CTRl.h"
extern void Rk3588_Printf(const char *strOutputString, ...);
extern __IO uint16_t adc1_ordinary_valuetab[2]; // 获取两个电机的ADC值

typedef struct
{
    enum
    {
        end,
        running,
        retract,
        error,
        manual
    } state;
    int8_t manual_Dir_last;
    int8_t manual_dir;
    uint8_t inc_dir;
    uint32_t Tick;
    uint32_t ovcT;
    uint32_t MaxCount;
    float angleReq;
    // float current_angle;
} Inc_Machine_parameter;
Inc_Machine_parameter inc_para;

extern uint8_t JY60_Get_Inc(float *angle);
extern float JY60_Get_IncAsync();

uint8_t Inc_ctrl(void)
{
    const int16_t angle_nomovecounterMax = 10000;
    static uint8_t Angle_nonreqTime;
    static int16_t last_angle, angle_nomovecounter;
    static float angle = 0;
    angle = JY60_Get_IncAsync();
    uint8_t exit_flag = 0;

    if (inc_para.state == running)
    {
        int16_t bias = ((inc_para.angleReq - angle) * 10.0f);
        int error = last_angle - bias;
        if (error == 0)
        {
            angle_nomovecounter++;
        }
        else
            angle_nomovecounter = 0;
        if (angle_nomovecounter > angle_nomovecounterMax)
        {
            exit_flag = 1;
        }
        last_angle = bias;
        Angle_nonreqTime = bias;

        // over current
        if (overCurrent()) // 过流
        {
            inc_para.ovcT++;
            //            INC_IO_Set(0); // 继电器停止
            //            Motor_Ctl_Set(0, 0);
            //            exit_flag = 1; // 退出
        }
        if (inc_para.ovcT > 500)
        {
            INC_IO_Set(0); // 继电器停止
            Motor_Ctl_Set(0, 0);
            exit_flag = 1; // 退出
            inc_para.ovcT = 0;
        }
        if (bias > 0)
        {
            Motor_Ctl_Set(1000, 1000);
            INC_IO_Set((inc_para.inc_dir)
                           ? 2
                           : 1);
        }
        else if (bias < 0)
        {
            Motor_Ctl_Set(-1000, -1000);
            INC_IO_Set((inc_para.inc_dir)
                           ? 1
                           : 2);
        }
        else
        {
            Motor_Ctl_Set(0, 0);
            INC_IO_Set(0);
            if (inc_para.angleReq < 0.5)
            {
                 inc_para.Tick=0;
                inc_para.state = retract;
            }
            else
            {

                exit_flag = 1;
                // inc_para.state = end;
            }
        }
    }
    if (inc_para.state == retract)
    {
        inc_para.Tick++;
        INC_IO_Set(2);
        Motor_Ctl_Set(-1000, -1000);
        if (inc_para.Tick > 3000)
        {
            Motor_Ctl_Set(0, 0);
            exit_flag = 1;
        }
    }
    if (inc_para.state == manual)
    {
        uint8_t io_cmd = (inc_para.manual_dir == 1) ? 1 : (inc_para.manual_dir == 0) ? 0
                                                      : (inc_para.manual_dir == -1)  ? 2
                                                                                     : 0;
        if (inc_para.inc_dir)
        {
            io_cmd = (inc_para.manual_dir == 1) ? 2 : (inc_para.manual_dir == 0) ? 0
                                                  : (inc_para.manual_dir == -1)  ? 1
                                                                                 : 0;
        }
        inc_para.Tick++;
        if (io_cmd == 1)
        {
            Motor_Ctl_Set(1000, 1000);
        }
        else
        {
            Motor_Ctl_Set(-1000, -1000);
        }
        INC_IO_Set(io_cmd);
        if (inc_para.Tick > 1000 || io_cmd != inc_para.manual_Dir_last)
        {
            inc_para.manual_Dir_last = io_cmd;
            exit_flag = 1;
            // inc_para.state = end;
        }
    }
    if (exit_flag == 1)
    {
        return 1;
    }
    return 0;
}

uint8_t INC_handler(void)
{
    uint8_t res = 0;
    res = Inc_ctrl();
    if (res)
    {
        INC_IO_Set(0);
        Motor_Ctl_Set(0, 0);
        inc_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t INC_Machine_Init(float angle, uint32_t Maxcount, uint8_t inc_dir)
{
#ifndef HARDWARE_TEST
#else
#endif
    INC_IO_INIT();
    Slave1_Set_Machine_Cb(INC_handler);
    inc_para.state = running;
    inc_para.angleReq = angle;
    inc_para.inc_dir = inc_dir;
    return 1;
}

uint8_t INC_Machine_Manual_Ctrl(int8_t direction, uint8_t inc_dir)
{
#ifndef HARDWARE_TEST
#else
#endif
    if (Slave1_Get_Machine_state())
        return 0;
    inc_para.inc_dir = inc_dir;
    if (inc_para.state == end)
    {
        inc_para.state = manual;
        inc_para.Tick = 0;
        INC_IO_INIT();
        Slave1_Set_Machine_Cb(INC_handler);
    }
    inc_para.Tick = 0;
    inc_para.manual_dir = direction;
    return 1;
}

uint8_t INC_Machine_Stop(void)
{
    inc_para.state = end;
    return 1;
}

uint8_t INC_Machine_Get_Count(float *angleReq, float *CurrentAngle)
{
    if (inc_para.state == end)
        return 0;
    if (angleReq != 0)
        *angleReq = inc_para.angleReq;
    if (CurrentAngle != 0)
        JY60_Get_Inc(CurrentAngle);
    return 1;
}
