/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-23 15:56:42
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\Task\task_info_struct.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef  __task_info_struct_H_
#define __task_info_struct_H_
#include "stdint.h"
#ifdef __cplusplus
extern "C"
{
#endif //

    #pragma anon_unions
    typedef enum
    {
        Task_VOR=0,
        Task_Continue,
        Task_OVAR,
        Task_VHIT,
        Task_TC,
    }Task_Parameter_Mode_enum;
    typedef enum
    {
        Ex_VOR=0,
        Ex_OKR,
        Ex_BOTH,
        Ex_BOTH_R
    }Task_Parameter_VOR_Exmode;
    //VOR
    typedef struct {
        uint32_t Counter;
        float Vel;
        float Freq;
        Task_Parameter_VOR_Exmode ExMode;
    }Task_Parameter_VOR;
    //CONT
    typedef struct {
        int Sec;
        float Vel;
        float okr_Vel;
    }Task_Parameter_Continue;
    //OVAR
    typedef struct {
        int Sec;
        float Vel;
        float Inc_Degree;
    }Task_Parameter_OVAR;
    //VHIT
    typedef struct {
        uint32_t Counter;
        uint8_t IsCW;
    }Task_Parameter_VHIT;
    //TC
    typedef struct {
        uint32_t Counter;
        float Vel;
    }Task_Parameter_TC;



    typedef struct
    {
        Task_Parameter_Mode_enum mode;
        union
        {
            Task_Parameter_VOR VOR;
            Task_Parameter_Continue CONT;
            Task_Parameter_OVAR OVAR;
            Task_Parameter_VHIT VHIT;
            Task_Parameter_TC TC;
        };
    }Task_Parameter_Struct;


#ifdef __cplusplus
}
#endif // 

#endif // 
