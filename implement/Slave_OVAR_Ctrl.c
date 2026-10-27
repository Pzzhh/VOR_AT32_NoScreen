/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-17 17:20:35
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE_
 */
#include "at32f435_437.h"
#include "Slave_OVAR_Ctrl.h"
#include "motor_para_cal.h"
#include "math.h"
#include "delay.h"
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"

#define PauseResetTick (1)

typedef struct
{
    enum
    {
        running,
        back,
        end,
        pause,
        IO_ready = 128
    } state;
    uint32_t Tick;
    float vel;
    // Task_Parameter_Struct info;
    uint32_t CurrentMillSec;
    int32_t accTimer;
    uint8_t ReqPause;
    uint32_t MillSecReq;
    float okr_last_Pos;
    Speed_Cal_Struct back;
} Ovar_Machine_parameter;
Ovar_Machine_parameter ovar_para;
#define Pi 3.1415926
// static float speeds = 0;

void OVAR_Back_init(float tragetPos, float sps, float accMs);
extern float Slave1_Get_Encode_Angle();
static uint8_t motor_set(void)
{
    uint8_t res;
    if (ovar_para.state & IO_ready)
    {
        // sla
        Slave1_TIM(0);
        Slave1_En_IO(1);
        ovar_para.Tick++;
        if (ovar_para.Tick > 1000)
        {
            ovar_para.Tick = 0;
            ovar_para.state &= (~IO_ready);
        }
    }
    if (ovar_para.state == running)
    {
        ovar_para.accTimer = (ovar_para.accTimer >= 1000) ? 1000 : ovar_para.accTimer + 1;
        ovar_para.Tick++;
        float factor = ovar_para.accTimer / 1000.0f;
        // if(Slave1_Get_Encode_Angle())
        // if (Slave1_Get_Encode_Angle())
        // {
        //     HAL_CAM_SET_sign_led();
        // }
        factor = (factor > 1) ? 1 : factor;
        tim_f_sin_set(angle_step * ovar_para.vel * factor);
        if (ovar_para.Tick >= ovar_para.MillSecReq || ovar_para.ReqPause)
        {
            ovar_para.state = back;
            return 0;
        }
    }
    if (ovar_para.state == back)
    {
        ovar_para.accTimer -= 1;
        tim_f_sin_set(angle_step * ovar_para.vel * ovar_para.accTimer / 1000.0f);
        if (ovar_para.accTimer <= 0)
        {
            tim_f_sin_set(0);
            Slave_Release();
            if (ovar_para.ReqPause)
            {
                ovar_para.state = pause;
                ovar_para.ReqPause = 0;
                return 0;
            }
            return 1; // END
        }
    }
    if (ovar_para.state == pause)
    {
        Slave_Release();
        if (ovar_para.ReqPause)
        {
            ovar_para.ReqPause = 0;
            ovar_para.state = running;
#if (PauseResetTick)
            ovar_para.Tick = 0; // wh
#endif
        }
    }
    return 0;
}
static uint8_t OVAR_handler(void)
{
    uint8_t res = 0;
    res = motor_set();
    if (res)
    {
        ovar_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t OVAR_Machine_Init(float vel, uint32_t MillSec)
{

#if 0
    Slave1_Set_Machine_Cb(CONT_handler);
    ovar_para.Tick = 0;
    ovar_para.state = running;
    ovar_para.MillSecReq = 2000;
#else
    if (vel > 360 || vel < -360)
        return 0;
    ovar_para.vel = vel;
    ovar_para.MillSecReq = MillSec;
    ovar_para.Tick = 0;
    ovar_para.accTimer = 0;
    ovar_para.ReqPause = 0;
    ovar_para.state = running | IO_ready;
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(OVAR_handler);
#endif
    return 1;
}

uint8_t OVAR_Machine_Stop(void)
{
    if (ovar_para.state == end)
        return 1;
    ovar_para.ReqPause = 0;
    ovar_para.state = back;
    return 1;
}

uint8_t OVAR_Machine_Pause(void)
{
    ovar_para.ReqPause = 1;
    return 0;
}

#define abs(n) ((n > 0) ? (n) : (-(n)))
void OVAR_Back_init(float tragetPos, float sps, float accMs)
{
    float speed, pos = Slave1_Get_Encode_Angle();
    ovar_para.back.motor.ms_esccape = 0;
    ovar_para.back.acc_time = accMs;
    ovar_para.back.sps = sps;
    if (tragetPos > pos)
        ovar_para.back.dir = 0;
    else
        ovar_para.back.dir = 1;

    ovar_para.back.lenght = abs(tragetPos - pos);
    motor_speed_cal(1, &speed, &ovar_para.back);
}

uint8_t OVAR_Machine_Get_Count(uint32_t *MillSecReq, uint32_t *CurrentMillSec)
{
    if (ovar_para.state == end)
        return Imp_finsih;
    if (MillSecReq != 0)
        *MillSecReq = ovar_para.MillSecReq;
    if (CurrentMillSec != 0)
        *CurrentMillSec = ovar_para.Tick;
    if (ovar_para.ReqPause == 1)
        return Imp_pausing;
    if (ovar_para.state == pause)
        return Imp_paused;
    return Imp_running;
}
