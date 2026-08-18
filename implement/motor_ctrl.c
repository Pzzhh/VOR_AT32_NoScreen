/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-08-02 10:50:44
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-02 11:45:07
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\motor_ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "motor_para_cal.h"
#include "string.h"
#include "math.h"
#define bytes_2_int16(b, a) ((int16_t)(a) | (b) << 8)
#define ISEXCEED(a, B, C) ((a > B && a > C) ? 1 : (a < B && a < C) ? 1 \
                                                                   : 0)
#define ISEXCEEDABS(a, B) ((a > B && a > -B) ? 1 : (a < B && a < -B) ? 1 \
                                                                     : 0)
#define K (1000)
#define M (K * K)

// return 1 runing return 0 end
uint8_t motor_speed_cal(uint8_t InitFlag, float *motor_speed, Speed_Cal_Struct *par)
{
    if (InitFlag)
    {
        // init
        memset(&par->motor, 0, sizeof(par->motor));
        float acc = (float)par->sps / (par->acc_time); // 每次加多少
        float acc_dis = par->sps * (par->acc_time);    //*2 计算加速距离
        float length = par->lenght;
        float sps = par->sps;
        par->motor.acc = acc; // set acc to mirco second acc->unit cm/s^2 par->motor.acc->unit cm/ms^2
        par->motor.sps = sps;
        if (acc_dis > par->lenght)
        {
            float acc_t = sqrt(length / (acc)); // 注意使用的时间单位对齐
            par->motor.end_tag = acc_t * 2;
            par->motor.up_tag = acc_t;
            par->motor.down_tag = acc_t;
        }
        else
        {
            float acc_t = sqrt(acc_dis / (acc));
            float plain_t = (length - acc_dis) / sps;
            par->motor.end_tag = acc_t * 2 + plain_t;
            par->motor.up_tag = acc_t;
            par->motor.down_tag = acc_t + plain_t;
        }
    }
    par->motor.esccape =  par->motor.ms_esccape / 1000;
    if (par->motor.esccape < par->motor.up_tag)
    {
        if (par->motor.speed < par->motor.sps)
        {
            par->motor.speed = par->motor.acc * par->motor.esccape;
            *motor_speed = par->motor.speed;
        }
    }
    else if (par->motor.esccape > par->motor.down_tag)
    {
        if (par->motor.speed > 0 && par->motor.esccape < par->motor.end_tag)
        {
            par->motor.speed = (par->motor.end_tag - par->motor.esccape) * par->motor.acc;
            *motor_speed = par->motor.speed;
        }
        else
            return 0;
    }
    return 1;
}
