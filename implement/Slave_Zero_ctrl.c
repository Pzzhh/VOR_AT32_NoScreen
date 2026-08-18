/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-10 10:09:51
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_Zero_ctrl.h"
#include "motor_para_cal.h"
#include "math.h"
#include "delay.h"
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"
#include "../HARDWARE/IR_Sense/IR.h"

typedef struct
{
    enum
    {
        running,
        back,
        end,
    } state;
    uint32_t Tick;
    uint32_t MillSecReq;
    uint16_t accTimer;
    uint8_t ioflag;
		uint16_t zeroIRCNT;
		uint16_t zeroNRCNT;
    // uint8_t backtick;
    float vel;
    Speed_Cal_Struct back;
} Zero_Machine_parameter;
Zero_Machine_parameter zero_para;
static const int backtick = (13 + 83) * 50 - 1000;

void Zero_Back_init(float len, float sps, float accMs);

static uint8_t slave_motivate()
{
    if (zero_para.Tick < zero_para.MillSecReq)
    {
        zero_para.accTimer = (zero_para.accTimer >= 1000) ? 1000 : zero_para.accTimer + 1;
    }
    else
    {
        zero_para.accTimer = (zero_para.accTimer == 0) ? 0 : zero_para.accTimer - 1;
    }
    zero_para.Tick++;
    float factor = zero_para.accTimer / 1000.0f;
    factor = (factor > 1) ? 1 : factor;
    tim_f_sin_set(angle_step *zero_para.vel* factor);
    if (zero_para.accTimer == 0)
    {
        return 1;
    }
    return 0;
}

static void slave_begin(float spd, uint32_t MillSecReq)
{
    zero_para.accTimer = 0;
    zero_para.Tick = 0;
    zero_para.vel = -spd;
    zero_para.MillSecReq = MillSecReq;
    zero_para.ioflag = 0;
}

static void slave_Reqstop()
{
    zero_para.MillSecReq = 0;
}

static void slave_Back_Zero_init()
{
    float tick ,angle=	Slave1_Get_Encode_Angle() ;
	  float vel = 20,velabs=20;
		angle=(angle<0)?-angle:angle;
		tick=	Slave1_Get_Encode_Angle() * 1000;
			if(angle<20)
			{
				velabs=angle;
				vel=angle;
			}
    if (tick < 0)
        tick = -tick;
    else
        vel = -vel;
    tick = tick / velabs;
    slave_begin(vel, tick);
}

static uint8_t motor_set(void)
{
    float speed = 0;
    // if (IR_IO_Get())
    // {
    //     Slave1_Set_Encode(0);
    //     zero_para.state = back;
    //     Zero_Back_init(140, 30, 1);
    //     tim_f_set(angle_step * 0);
    // }
    if (zero_para.state == running)
    {
        uint8_t Flag = slave_motivate();
        // if(zero_para.ioflag<=2)
        if (IR_IO_Get() == 1 && zero_para.ioflag == 0)
        {
						zero_para.zeroIRCNT++;
           
        }else
				{
					zero_para.zeroIRCNT=0;
				}
				if(	zero_para.zeroIRCNT>20)
				{
					 Slave1_Check_Encode_Angle();
            Slave1_Set_Encode_Angle(-87.0f);
            slave_Reqstop();
            zero_para.ioflag = 1;
				}
        if (Flag == 1)
        {
            if (zero_para.ioflag)
            {
                zero_para.state = back;
                slave_Back_Zero_init();
            }
            else
            {
                return 1;
            }
        }
    }
    if (zero_para.state == back)
    {
			
			  Slave1_En_IO(1);
			if(zero_para.zeroNRCNT>1000)
			{
				uint8_t Flag = slave_motivate();
        if (Flag == 1)
        {
					zero_para.zeroNRCNT=0;
            return 1;
        }
			}else
			zero_para.zeroNRCNT++;
    }
    return 0;
}
static uint8_t Zero_handler(void)
{
    uint8_t res = 0;
    res = motor_set();
    if (res)
    {
        zero_para.state = end;
        
        tim_f_set(0);
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t Zero_Machine_get_flag(void)
{
    if (zero_para.state == end)
    {
        return 1;
    }
    return 0;
}

uint8_t Zero_Machine_Init(uint8_t isinit)
{
    float speed;
#if 0
    Slave1_Set_Machine_Cb(CONT_handler);
    zero_para.Tick = 0;
    zero_para.state = running;
    zero_para.MillSecReq = 2000;
#else
    memset(zero_para, 0, sizeof(zero_para));
    if (isinit)
    {
        Slave1_Check_Encode_Angle();
        IR_IO_Init();
        Zero_Back_init(360, 30, 1);
    }
    else
    {
        zero_para.state = back;
        slave_Back_Zero_init();
    }
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(Zero_handler);
#endif
    return 1;
}

void Zero_Back_init(float len, float sps, float accMs)
{
    zero_para.vel = 20;
    zero_para.MillSecReq = 18 * 1000;
    zero_para.state = running;
    // float speed;
    // zero_para.back.motor.ms_esccape = 0;
    // zero_para.back.acc_time = accMs;
    // zero_para.back.sps = sps;
    // zero_para.back.dir = 1;
    // zero_para.back.lenght = len;
    // motor_speed_cal(1, &speed, &zero_para.back);
}
