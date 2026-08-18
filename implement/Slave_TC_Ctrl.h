/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:29
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-01-17 11:08:53
 * @FilePath: \USERd:\workfile\项目3 vor\software\TC_V2.0\implement\Slave_TC_Ctrl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __Slave_TC_Ctrl_H_
#define __Slave_TC_Ctrl_H_
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t TC_Machine_Init(uint16_t ReqCNT, float vel);
    uint8_t TC_Machine_Get_Count(uint32_t *counterReq, uint32_t *CurrentCounter);
    uint8_t TC_Machine_Stop(void);
    uint8_t TC_Machine_Pause(void);

#ifdef __cplusplus
}
#endif

#endif