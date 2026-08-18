/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-06-26 11:22:09
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-02 11:38:41
 * @FilePath: \平衡台_AT32_V1.1\inc\motor_ctrl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __motor_para_cal_H
#define __motor_para_cal_H
#include "stdint.h"
typedef struct
{
    uint8_t dir;
    float sps;      // step per second
    float acc_time; // accalrate time
    float lenght;   //
    struct
    {
        // flag
        // counter
        uint32_t ms_esccape;
        float esccape;
        float up_tag;
        float down_tag;
        float end_tag;
        // parameter
        float sps;
        float acc; // par micro second
        // output
        float speed;
    } motor;
} Speed_Cal_Struct;


uint8_t motor_speed_cal(uint8_t InitFlag, float *motor_speed, Speed_Cal_Struct *par);

#endif
