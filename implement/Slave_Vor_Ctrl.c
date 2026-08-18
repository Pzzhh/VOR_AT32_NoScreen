/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-11-20 14:40:41
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_Vor_Ctrl.h"
#include "math.h"
#include "delay.h"
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"

#define PauseResetTick (1)

int sin_time = 0;
int tim_count, last_count, plus_f = 1;
extern void Motor_Set_Speed(float speed);
typedef enum
{
    Ex_VOR = 0,
    Ex_OKR,
    Ex_BOTH,
    Ex_BOTH_R
} VOR_Exmode;
typedef struct
{
    enum
    {
        running,
        end,
        pause,
        IO_ready = 128
    } state;
    uint8_t RepPause;
    uint8_t RepStop;
    uint16_t PauseCount;
    float pausePhase;
    uint32_t Tick;
    float freq;
    float vel;
    uint8_t ExMode;
    // Task_Parameter_Struct info;
    uint32_t CurrentCounter;
    uint32_t counterReq;
} Vor_Machine_parameter;
Vor_Machine_parameter vor_para;
#define Pi 3.1415926
const uint16_t PauseStopCount = 500;

int C610SpdCompensetion = 568;
extern uint8_t HAL_CAM_SET_sign_led(void);

void MotorSpeedSet(float sin_data, float factor)
{
    if (vor_para.ExMode == Ex_BOTH ||
        vor_para.ExMode == Ex_OKR)
        Motor_Set_Speed(-vor_para.vel * sin_data * factor * ((float)C610SpdCompensetion / 500.0f));
    else if (vor_para.ExMode == Ex_BOTH_R)
        Motor_Set_Speed(vor_para.vel * sin_data * factor * ((float)C610SpdCompensetion / 500.0f));
    else
        Motor_Set_Speed(0);

    if (vor_para.ExMode != Ex_OKR)
        tim_f_sin_set(angle_step * sin_data * vor_para.vel * factor);
    else
        tim_f_sin_set(0);
}

uint8_t Slave_motor(void)
{
    float time_max = 1000;
    float sin_data;
    if (vor_para.RepStop)
        return 1;
    if (vor_para.state & IO_ready)
    {
        Slave1_TIM(0);
        Slave1_En_IO(1);
        vor_para.Tick++;
        if (vor_para.Tick > 1000)
        {
            vor_para.Tick = 0;
            vor_para.state &= (~IO_ready);
        }
    }
    if (vor_para.state == running)
    {
        uint32_t Phase_2Pi = 1000.0f / vor_para.freq;
        uint32_t Phase_Pi = 500.0f / vor_para.freq;
        if (vor_para.Tick % Phase_2Pi == 0)
        {
            HAL_CAM_SET_sign_led();
        }
        float factor = (vor_para.Tick >= 1000) ? 1.0f : vor_para.Tick / 1000.0f;
        factor = (factor > 1) ? 1 : factor;
        sin_data = sin(((float)2.0f * Pi * vor_para.freq * vor_para.Tick / 1000.0f));
        vor_para.CurrentCounter = vor_para.freq * vor_para.Tick / 1000.0f;
        vor_para.Tick++;
        MotorSpeedSet(sin_data, factor); // motor speed set

        if (vor_para.CurrentCounter >= vor_para.counterReq)
            return 1;                       // **finish
        if (vor_para.Tick % Phase_2Pi == 0) // Enter pause when return Phase Zero
        {
            if (vor_para.RepPause)
            {
                vor_para.state = pause;
                vor_para.RepPause = 0;
            }
        }
    }
    if (vor_para.state == pause)
    {
        Slave_Release();
        if (vor_para.RepPause)
        {
            vor_para.state = running;
            vor_para.RepPause = 0;
#if (PauseResetTick)
            vor_para.Tick = 0; // when exit pause stage reset current Tick
#endif
        }
    }
    return 0; //**normal Exit
}

uint8_t VOR_handler(void)
{
    uint8_t res = 0;
    res = Slave_motor();
    if (res)
    {
        Motor_Set_Speed(0);
        vor_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t VOR_Machine_Init(float freq, float vel, uint32_t count, uint8_t Exmode)
{
#ifdef HARDWARE_TEST
    Slave1_Set_Machine_Cb(VOR_handler);
    vor_para.Tick = 0;
    vor_para.state = running;
    vor_para.counterReq = 2;
#else
    if (vel > 360 ||
        freq <= 0 || freq >= 15)
        return 0;
    vor_para.freq = freq;
    vor_para.vel = vel;
    vor_para.counterReq = count;
    vor_para.ExMode = Exmode;
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(VOR_handler);
    vor_para.Tick = 0;
    vor_para.state = running | IO_ready;
    vor_para.RepPause = 0;
    vor_para.RepStop = 0;
#endif
    return 1;
}

uint8_t VOR_Machine_Stop(void)
{
    vor_para.counterReq = vor_para.CurrentCounter + 1;
    vor_para.RepStop = 1;
    return 1;
}

uint8_t VOR_Machine_Pause(void)
{
    vor_para.RepPause = 1;
    return 1;
}

uint8_t VOR_Machine_Get_Count(uint32_t *counterReq, uint32_t *CurrentCounter)
{
    if (vor_para.state == end)
        return Imp_finsih;
    if (counterReq != 0)
        *counterReq = vor_para.counterReq;
    if (CurrentCounter != 0)
        *CurrentCounter = vor_para.CurrentCounter;
    if (vor_para.RepPause == 1 && vor_para.state == running)
        return Imp_pausing;
    if (vor_para.state == pause)
        return Imp_paused;
    return Imp_running;
}
