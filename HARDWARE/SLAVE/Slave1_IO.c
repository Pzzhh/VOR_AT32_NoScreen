/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:47:04
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 16:34:13
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\SLAVE\Slave1_IO.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Slave1_IO.h"
#include "at32f435_437.h"

void Slave1_IO_Init(void)
{
    CRM->ahben1_bit.gpioaen = 1;
    CRM->ahben1_bit.gpiocen = 1;
    CRM->ahben1_bit.gpioben = 1;

    GPIOA->cfgr_bit.iomc3 = 1; // output mode for alm clr
    GPIOA->cfgr_bit.iomc4 = 0; // input mode for
    GPIOA->cfgr_bit.iomc6 = 1; // output mode for alm
    GPIOA->cfgr_bit.iomc7 = 1; // output mode for svr_dir
    GPIOA->cfgr_bit.iomc3 = 1; // output mode for

}

void Slave1_TIM(uint8_t state)
{
    TMR1->ctrl1_bit.tmren = state;
}

void Slave1_En_IO(uint8_t state)
{
    uint16_t *const gpio_reg = (uint16_t *)&GPIOA->scr;
    const uint16_t gpio_pin = GPIO_PINS_3;
    if (state)
    {
        gpio_reg[0] = gpio_pin;
    }
    else
    {
        gpio_reg[1] = gpio_pin;
    }
}

void Slave1_Dir_IO(uint8_t state)
{
    uint16_t *const gpio_reg = (uint16_t *)&GPIOA->scr;
    const uint16_t gpio_pin = GPIO_PINS_7;
    if (!state)
    {
        gpio_reg[0] = gpio_pin;
    }
    else
    {
        gpio_reg[1] = gpio_pin;
    }
}
