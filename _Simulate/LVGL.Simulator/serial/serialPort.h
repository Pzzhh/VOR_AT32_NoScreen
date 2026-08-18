#ifndef  __SerialPort_H_
#define __SerialPort_H_
#ifdef __cplusplus
extern "C"
{
#endif //
#include "stdint.h"
#include <windows.h>
    void SerialPort_Init(void (*cb)(uint8_t dr), LPCWSTR comname);
    void SerialPort_SendBuf(uint8_t* buf, uint16_t len);
#ifdef __cplusplus
}
#endif // 

#endif // 

