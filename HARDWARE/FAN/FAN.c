/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:36
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 12:11:09
 * @FilePath: \Slave_At32\HARDWARE\FAN\FAN.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "FAN.h"
#include "at32f435_437.h"

void FAN_Init()
{
    CRM->apb1en_bit.tmr4en = 1;
    CRM->ahben1_bit.gpioben = 1;

    TMR4->c1dt = 0;

    GPIOB->cfgr_bit.iomc6 = 2; // AF2 for TMR4_CH1 on PB6
    // Alternate function mode for PB6


    GPIOB->muxl |= 0x2 << (4 * (6)); // PB6 AF2

    // Set the prescaler value to have TMR4 counter clock = 1 MHz
    TMR4->pr = 42 - 1; // APB1 clock is 84 MHz / 2 = 42 MHz

    // Set the ARR value to have a PWM frequency of 1 kHz
    TMR4->rpr = 1000 - 1;

    // Set the CCR1 value to have a duty cycle of 50%
    TMR4->c1dt = 500;

    // Configure the TMR4 channel 1 in PWM mode 1
    TMR4->cm1_output_bit.c1octrl = 6; // PWM mode 1

    // Enable the output compare 1 preload register
    TMR4->cm1_output_bit.c1oien = 1;

    // Enable the capture/compare 1 output
    TMR4->cctrl_bit.c1en = 1;

    // Enable auto-reload preload
    TMR4->ctrl1_bit.prben = 1;

    // Enable the TMR4 main output
    TMR4->brk_bit.aoen = 1;

    // Enable the TMR4 counter
    TMR4->ctrl1_bit.tmren = 1;
}

void FAN_Ctrl(uint8_t pwn_precent)
{
    TMR4->c1dt = pwn_precent*10;
}
