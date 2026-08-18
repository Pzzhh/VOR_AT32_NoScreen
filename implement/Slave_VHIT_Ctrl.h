/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:29
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-02-10 13:46:35
 * @FilePath: \USERd:\workfile\项目3 vor\software\VHIT_V2.0\implement\Slave_Vor_Ctrl.h
 * @Description: 这是默认设置,请设置`customMade`,
打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __Slave_VHIT_Ctrl_H_
#define __Slave_VHIT_Ctrl_H_
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t VHIT_Machine_Init(uint16_t ReqCNT, uint8_t isCW);
    uint8_t VHIT_Machine_Get_Count(uint32_t *counterReq,
                                   uint32_t *CurrentCounter);
    uint8_t VHIT_Machine_Stop(void);
    uint8_t VHIT_Machine_Pause(void);
    uint8_t VHIT_Machine_Next(void);
#define VHIT_Vel_Array_Size (201)
    extern const float VHIT_Vel_Array[201];

#ifdef __cplusplus
}
#endif

#endif