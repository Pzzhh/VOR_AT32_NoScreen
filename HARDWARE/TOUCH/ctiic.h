/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:36
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 11:18:53
 * @FilePath: \Slave_At32\HARDWARE\TOUCH\ctiic.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __MYCT_IIC_H
#define __MYCT_IIC_H
#include "at32f435_437.h"
//////////////////////////////////////////////////////////////////////////////////
// 本程序只供学习使用，未经作者许可，不得用于其它任何用途
// ALIENTEK STM32F407开发板
// 电容触摸屏-IIC 驱动代码
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/7
// 版本：V1.1
// 版权所有，盗版必究。
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//********************************************************************************
// 修改说明
// V1.1 20140721
// 1,修改CT_IIC_Read_Byte函数,读数据更快.
// 2,修改CT_IIC_Wait_Ack函数,以支持MDK的-O2优化.
//////////////////////////////////////////////////////////////////////////////////

// IO方向设置 (AT32: 使用GPIOF->cfgr配置模式位)
#define CT_SDA_IN()                       \
    do                                    \
    {                                     \
        GPIOF->cfgr &= ~(3u << (2 * 11)); \
        GPIOF->cfgr |= (0u << (2 * 11));  \
    } while (0) // PF11输入模式
#define CT_SDA_OUT()                      \
    do                                    \
    {                                     \
        GPIOF->cfgr &= ~(3u << (2 * 11)); \
        GPIOF->cfgr |= (1u << (2 * 11));  \
    } while (0) // PF11输出模式
// IO操作函数
#define CT_IIC_SCL GPIOB->odt_bit.odt0   // SCL
#define CT_IIC_SDA GPIOF->odt_bit.odt11  // SDA
#define CT_READ_SDA GPIOF->idt_bit.idt11 // 输入SDA

// IIC所有操作函数
void CT_IIC_Init(void);                 // 初始化IIC的IO口
void CT_IIC_Start(void);                // 发送IIC开始信号
void CT_IIC_Stop(void);                 // 发送IIC停止信号
void CT_IIC_Send_Byte(u8 txd);          // IIC发送一个字节
u8 CT_IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
u8 CT_IIC_Wait_Ack(void);               // IIC等待ACK信号
void CT_IIC_Ack(void);                  // IIC发送ACK信号
void CT_IIC_NAck(void);                 // IIC不发送ACK信号

#endif
