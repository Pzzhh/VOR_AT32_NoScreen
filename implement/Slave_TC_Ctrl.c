/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-03-28 16:52:56
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_TC_Ctrl.h"
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
        reverse,
        slowdown,
        intval,
        end,
        pause
    } state;
    uint32_t Tick;
    float vel;
    uint32_t accTimer;
    uint32_t MillSecReq;
    uint8_t ReqPause;
    uint8_t ReqStop;
    uint8_t isCW;
    uint16_t ReqCNT;
    uint16_t CurrentCNT;
} TC_Machine_parameter;
TC_Machine_parameter tc_para;
#define Pi 3.1415926
static const float TC_ACC_Tick = 100;

void TC_Back_init(float tragetPos, float sps, float accMs);

static uint8_t motor_set(void)
{
    uint8_t res;
    if (tc_para.state == running)
    {
        tc_para.accTimer = (tc_para.accTimer >= TC_ACC_Tick) ? TC_ACC_Tick : tc_para.accTimer + 1;
        tc_para.Tick++;
        float factor = tc_para.accTimer / TC_ACC_Tick;
        factor = (factor > 1) ? 1 : factor;
        tim_f_sin_set(angle_step * tc_para.vel * factor);
        if (tc_para.Tick >= tc_para.MillSecReq || tc_para.ReqPause || tc_para.ReqStop)
        {
            tc_para.state = slowdown;
            return 0;
        }
    }
    if (tc_para.state == slowdown)
    {
        tc_para.accTimer -= 1;
        tim_f_sin_set(angle_step * tc_para.vel * tc_para.accTimer / TC_ACC_Tick);
        if (tc_para.accTimer <= 0)
        {
            tim_f_sin_set(0);
            Slave_Release();
            tc_para.CurrentCNT++;
            if (tc_para.ReqStop ||
                tc_para.CurrentCNT >= tc_para.ReqCNT)
            {
                tc_para.state = end;
                return 1; // END
            }
            tc_para.Tick = 0;
            tc_para.state = intval;
        }
    }
    if (tc_para.state == intval)
    {
        tc_para.Tick++;
        if (tc_para.Tick >= 5000)
        {
            if (tc_para.CurrentCNT >= tc_para.ReqCNT)
            {
                tc_para.state = end;
                return 1;
            }
            tc_para.Tick = 0;
            tc_para.state = running;
        }
        if (tc_para.ReqPause)
        {
            tc_para.state = pause;
            tc_para.ReqPause = 0;
            return 0;
        }
        if (tc_para.ReqStop)
        {
            tc_para.state = end;
            tc_para.ReqStop = 0;
            return 1;
        }
    }
    if (tc_para.state == pause)
    {
        Slave_Release();
        if (tc_para.ReqPause)
        {
            tc_para.ReqPause = 0;
            tc_para.state = running;
            tc_para.CurrentCNT = 0;
#if PauseResetTick
            tc_para.Tick = 0;
#endif
        }
    }
    return 0;
}

static uint8_t TC_handler(void)
{
    uint8_t res = 0;
    res = motor_set();
    if (res)
    {
        tc_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t TC_Machine_Init(uint16_t ReqCNT, float vel)
{
#if 0
    Slave1_Set_Machine_Cb(TC_handler);
    tc_para.Tick = 0;
    tc_para.state = running;
    tc_para.MillSecReq = 2000;
#else
    memset(tc_para, 0, sizeof(tc_para));
    tc_para.ReqCNT = ReqCNT;
    tc_para.vel = vel;
    tc_para.MillSecReq = 30 * 1000;
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(TC_handler);
    tc_para.Tick = 0;
    tc_para.state = running;
#endif
    return 1;
}

uint8_t TC_Machine_Stop(void)
{
    tc_para.ReqStop = 1;
    return 0;
}

uint8_t TC_Machine_Pause(void)
{
    tc_para.ReqPause = 1;
    return 0;
}

uint8_t TC_Machine_Get_Count(uint32_t *CNTreq, uint32_t *CurrentMillSec)
{
    if (tc_para.state == end)
        return Imp_finsih;
    if (CNTreq != 0)
        *CNTreq = tc_para.MillSecReq;
    if (CurrentMillSec != 0)
        *CurrentMillSec = tc_para.Tick;
    if (tc_para.ReqPause == 1)
        return Imp_pausing;
    if (tc_para.state == pause)
        return Imp_paused;
    return Imp_running;
}
