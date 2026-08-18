/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:36
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 11:18:53
 * @FilePath: \Slave_At32\HARDWARE\TOUCH\ctiic.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __INC_MOTOR_H
#define __INC_MOTOR_H
#include "at32f435_437.h"

void motor_init(void);
void Motor_Ctl_Set(int16_t motor1, int16_t motor2);
uint8_t overCurrent(void);

#endif
