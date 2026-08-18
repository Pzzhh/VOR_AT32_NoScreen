#ifndef __Slave1_IO_H_
#define __Slave1_IO_H_
#include "stdint.h"
#ifdef __cplusplus
extern "C"
{
#endif
    void Slave1_IO_Init(void);
    void Slave1_TIM(uint8_t state);
    void Slave1_En_IO(uint8_t state);
    void Slave1_Dir_IO(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif
