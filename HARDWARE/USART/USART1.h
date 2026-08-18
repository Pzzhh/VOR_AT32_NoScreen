/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-31 14:28:53
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-08-02 18:24:08
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\USART\USART1.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __USART1_H_
#define __USART1_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
    void USART1_INIT(
        uint32_t boundrate,
        void (*handle)(uint8_t dr));

    uint8_t USART1_Send_Package(char *string, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif