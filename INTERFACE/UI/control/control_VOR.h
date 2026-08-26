#ifndef __CONTROL_VOR_H_
#define __CONTROL_VOR_H_

#include "stdint.h"
#include "control.h"
#include "control_base.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t HAL_Slave_VOR_Init(Task_Parameter_Struct *task);
uint8_t HAL_Slave_VOR_Stop(void);
uint8_t HAL_Slave_VOR_Pause(uint8_t enable);
uint8_t HAL_Slave_VOR_Get_State(uint32_t *remaining_count, uint32_t *percent);
const char *ModeStr(Task_Parameter_Struct *task);
uint16_t VorControlFunction(Task_Parameter_Struct *task, Task_control_info *control);
const Control_Driver *VOR_Get_Driver(void);

#ifdef __cplusplus
}
#endif

#endif
