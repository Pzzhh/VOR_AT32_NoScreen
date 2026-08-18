#ifndef __USART5_H_
#define __USART5_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
    void USART3_INIT(
        uint32_t boundrate,
        void (*handle)(uint8_t dr));

    uint8_t USART3_PRINTF(char *string, uint16_t size);
    uint8_t USART3_Send_Package(char *string, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif