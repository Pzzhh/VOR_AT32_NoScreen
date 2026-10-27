#ifndef __JY60_H_
#define __JY60_H_
#include "stdint.h"
#ifdef __cplusplus
extern "C"
{
#endif //
    
    void JY60_Init(void);
    uint8_t JY60_Get_Inc(float *angle);
    uint8_t JY60_Get_Inc_Fresh(float *angle, uint32_t max_age_ms);

#ifdef __cplusplus
}
#endif //

#endif //
