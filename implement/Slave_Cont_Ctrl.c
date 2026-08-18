/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-17 17:23:05
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_Cont_Ctrl.h"
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
        	IO_ready=128
    } state;
    uint32_t Tick;
    float vel;
		 float okr_vel;
    uint8_t ReqPause;
    // Task_Parameter_Struct info;
    int32_t accTimer;
    uint32_t MillSecReq;
    Speed_Cal_Struct back;
} Cont_Machine_parameter;
Cont_Machine_parameter cont_para;
#define Pi 3.1415926


extern void Motor_Set_Speed(float speed);
extern int C610SpdCompensetion;
void Cont_Back_init(float tragetPos, float sps, float accMs);

static uint8_t motor_set(void)
{
    uint8_t res;
     if (cont_para.state & IO_ready)
    {
        // sla
        Slave1_TIM(0);
        Slave1_En_IO(1);
        cont_para.Tick++;
        if (cont_para.Tick > 1000)
        {
            cont_para.Tick = 0;
            cont_para.state &= (~IO_ready);
        }
    }
    if (cont_para.state == running)
    {
        cont_para.accTimer = (cont_para.accTimer >= 1000) ? 1000 : cont_para.accTimer + 1;
        cont_para.Tick++;
        float factor = cont_para.accTimer / 1000.0f;
        factor = (factor > 1) ? 1 : factor;
        tim_f_sin_set(angle_step * cont_para.vel * factor);
				Motor_Set_Speed(-cont_para.okr_vel * factor * ((float)C610SpdCompensetion / 500.0f));
        if (cont_para.Tick >= cont_para.MillSecReq || cont_para.ReqPause)
        {
            cont_para.state = back;
            return 0;
        }
    }
    if (cont_para.state == back)
    {
        cont_para.accTimer -= 1;
        tim_f_sin_set(angle_step * cont_para.vel * cont_para.accTimer / 1000.0f);
        if (cont_para.accTimer <= 0)
        {
            tim_f_sin_set(0);
            Slave_Release();
            if (cont_para.ReqPause)
            {
                cont_para.state = pause;
                cont_para.ReqPause = 0;
                return 0;
            }
            return 1; // END
        }
    }
    if (cont_para.state == pause)
    {
        Slave_Release();
        if (cont_para.ReqPause)
        {
            cont_para.ReqPause = 0;
            cont_para.state = running;
#if PauseResetTick
            cont_para.Tick = 0;
#endif
        }
    }
    return 0;
}

static uint8_t CONT_handler(void)
{
    uint8_t res = 0;
    res = motor_set();
    if (res)
    {
        cont_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t CONT_Machine_Init(float vel,float okr_vel, uint32_t MillSec)
{
#if 0
    Slave1_Set_Machine_Cb(CONT_handler);
    cont_para.Tick = 0;
    cont_para.state = running;
    cont_para.MillSecReq = 2000;
#else
    if (vel > 360 || vel < -360)
        return 0;
		 if (okr_vel > 360 || okr_vel < -360)
        return 0;
    cont_para.vel = vel;
    cont_para.okr_vel = okr_vel;
    cont_para.MillSecReq = MillSec;
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(CONT_handler);
    cont_para.Tick = 0;
    cont_para.state = running|IO_ready;
    cont_para.accTimer = 0;
#endif
    return 1;
}

uint8_t Cont_Machine_Stop(void)
{
    cont_para.accTimer = 0;
    cont_para.state = back;
    return 0;
}

uint8_t Cont_Machine_Pause(void)
{
    cont_para.ReqPause = 1;
    return 0;
}

#define abs(n) ((n > 0) ? (n) : (-(n)))
void Cont_Back_init(float tragetPos, float sps, float accMs)
{
    float speed, pos = Slave1_Get_Encode_Angle();
    cont_para.back.motor.ms_esccape = 0;
    cont_para.back.acc_time = accMs;
    cont_para.back.sps = sps;
    if (tragetPos > pos)
        cont_para.back.dir = 0;
    else
        cont_para.back.dir = 1;

    cont_para.back.lenght = abs(tragetPos - pos);
    motor_speed_cal(1, &speed, &cont_para.back);
}

uint8_t Cont_Machine_Get_Count(uint32_t *MillSecReq, uint32_t *CurrentMillSec)
{
    if (cont_para.state == end)
        return Imp_finsih;
    if (MillSecReq != 0)
        *MillSecReq = cont_para.MillSecReq;
    if (CurrentMillSec != 0)
        *CurrentMillSec = cont_para.Tick;
    if (cont_para.ReqPause == 1)
        return Imp_pausing;
    if (cont_para.state == pause)
        return Imp_paused;
    return Imp_running;
}
