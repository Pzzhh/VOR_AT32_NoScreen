/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-08-05 11:22:48
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-05 11:27:53
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\IR_Sense\IR.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "IR.h"
#include "at32f435_437.h"
// 0:none 1:reach
uint8_t IR_IO_Get(void)
{
    // uint32_t res = GPIOE->IDR;
    // res &= GPIO_Pin_6;
    return (GPIOE->idt_bit.idt6 == 0) ? 1 : 0;
}

void IR_IO_Init(void)
{
    CRM->ahben1_bit.gpioeen = 1;
    GPIOE->cfgr_bit.iomc6 = 0; // input mode for
    GPIOE->pull_bit.pull6 = 1; // pull-up
}
