#ifndef __USART5_H_
#define __USART5_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
    void USART5_INIT(
        uint32_t boundrate,
        void (*handle)(uint8_t dr));
    uint8_t USART5_Send_Package(uint8_t *string, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif