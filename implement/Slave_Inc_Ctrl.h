/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-08-05 09:13:41
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2024-09-02 16:31:42
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Inc_Ctrl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __Slave_Inc_Ctrl_H_
#define __Slave_Inc_Ctrl_H_
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t INC_Machine_Init(float angle, uint32_t Maxcount, uint8_t inc_dir);
    uint8_t INC_Machine_Stop(void);
    uint8_t INC_Machine_Get_Count(float *angleReq, float *CurrentAngle);
    uint8_t INC_Machine_Manual_Ctrl(int8_t direction, uint8_t inc_dir);
    typedef enum {
        INC_RESULT_OK = 0, INC_RESULT_MOVING, INC_RESULT_STOPPED,
        INC_RESULT_SENSOR, INC_RESULT_OVERCURRENT, INC_RESULT_TIMEOUT,
        INC_RESULT_STALLED
    } INC_Result;
    INC_Result INC_Machine_Get_Result(void);
    
#ifdef __cplusplus
}
#endif

#endif
