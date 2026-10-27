/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:29
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-04-24 18:11:03
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\SLAVE\Slave1_Timer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __Slave1_Timer_H_
#define __Slave1_Timer_H_
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define motor_step 10000 * 10
#define angle_step (277.8f * 5)
    typedef uint8_t (*Slave_Function_CB)(void);
    typedef enum
    {
        Slave_CMD_N,   // normal_mode
        Slave_CMD_Sin, // sin
        Slave_CMD_Back_N,
        Slave_CMD_Back_Sin,
    } Slave1_CMD_Typed;
    typedef enum
    {
        Imp_finsih = 0,
        Imp_paused,
        Imp_pausing,
        Imp_running
    } Imp_State;
    void Slave1_Step_Generator_Init(uint32_t arr, uint32_t psc);
    void Slave1_Set_Machine_Cb(Slave_Function_CB cb);
    uint8_t Slave1_Get_Machine_state();
    void tim_f_sin_set(int f);
    void tim_f_sin_set_Dir(int f);
		uint8_t Slave1_get_Encode_error();
		uint8_t Slave1_Check_Encode_Angle(uint8_t isCheck);
    void tim_f_set(int f);
    void Slave_server_Init(void);
    float Slave1_Get_Encode_Angle(void);
		float Slave1_Get_Encode_Angle_real(void);
    int32_t Slave1_Get_Encode(void);
    void Slave1_Set_Encode(uint32_t cnt);
    void Slave1_Set_Encode_Angle(float angle);
#ifdef __cplusplus
}
#endif

#endif
