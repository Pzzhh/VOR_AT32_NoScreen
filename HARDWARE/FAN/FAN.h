#ifndef __FAN_H_
#define __FAN_H_

#include "stdint.h"

void FAN_Init(void);
void FAN_Ctrl(uint8_t pwn_precent);

#endif