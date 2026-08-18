#ifndef __control_VOR_H_
#define __control_VOR_H_
#include "stdint.h"
#ifdef __cplusplus
extern "C"
{
#endif //
    
    void JY60_Init(void);
    uint8_t JY60_Get_Inc(float *angle);

#ifdef __cplusplus
}
#endif //

#endif //
