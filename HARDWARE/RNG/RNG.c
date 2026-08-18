/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:36
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-10 11:52:48
 * @FilePath: \VOR_V3.0_AT32\HARDWARE\RNG\RNG.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "at32f435_437.h"
#include "RNG.h"
#include "stdlib.h"
// 0:none 1:reach
uint8_t RNG_Generate(void)
{
    return rand();
}

extern uint16_t adc1_ordinary_valuetab[2];

void RNG_Generate_Init(void)
{
    ertc_time_type rtc_time = {0};
    uint32_t seed;

    ertc_calendar_get(&rtc_time);

    seed = ((uint32_t)adc1_ordinary_valuetab[0] << 16)
         ^ (uint32_t)adc1_ordinary_valuetab[1]
         ^ ((uint32_t)rtc_time.sec * 0x9E3779B9UL);

    srand((unsigned int)seed);
}
