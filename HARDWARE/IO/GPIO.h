#ifndef _GPIO_H_
#define _GPIO_H_
#include "stdint.h"
void CAM_GPIO_INIT(void);
void GPIO_CAM_REC_IO_Ctrl(int flag);
void GPIO_CAM_SyncLED_IO_Ctrl();
void GPIO_CAM_Wifi_IO_Ctrl();
void GPIO_Beep_IO_Ctrl(uint32_t ms);

#endif