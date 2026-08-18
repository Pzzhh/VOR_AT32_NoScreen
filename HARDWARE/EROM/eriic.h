/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:36
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-09 16:41:40
 * @FilePath: \Slave_At32\HARDWARE\TOUCH\ctiic.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __MYER_IIC_H
#define __MYER_IIC_H
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
// 1,修改ER_IIC_Read_Byte函数,读数据更快.
// 2,修改ER_IIC_Wait_Ack函数,以支持MDK的-O2优化.
//////////////////////////////////////////////////////////////////////////////////

// IO方向设置 (AT32: 使用GPIOF->cfgr配置模式位)
// IO操作函数
#define ER_IIC_SCL GPIOG->odt_bit.odt7  // SCL
#define ER_IIC_SDA GPIOG->odt_bit.odt8  // SDA
#define ER_READ_SDA GPIOG->idt_bit.idt8 // 输入SDA

#define IIC_READ_SDA ER_READ_SDA

/* IO操作 */
#define IIC_SCL(x)      \
    {                   \
        ER_IIC_SCL = x; \
    } /* SCL */

#define IIC_SDA(x)      \
    {                   \
        ER_IIC_SDA = x; \
    } /* SDA */

// IIC所有操作函数
void ER_IIC_Init(void);                 // 初始化IIC的IO口
void ER_IIC_Start(void);                // 发送IIC开始信号
void ER_IIC_Stop(void);                 // 发送IIC停止信号
void ER_IIC_Send_Byte(u8 txd);          // IIC发送一个字节
u8 ER_IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
u8 ER_IIC_Wait_Ack(void);               // IIC等待ACK信号
void ER_IIC_Ack(void);                  // IIC发送ACK信号
void ER_IIC_NAck(void);                 // IIC不发送ACK信号
uint8_t ER_IIC_Write_Bytes(uint8_t I2C_Addr, uint16_t addr, uint8_t *buf, uint16_t len);
uint8_t ER_IIC_Read_Bytes(uint8_t I2C_Addr, uint16_t addr, uint8_t *buf, uint16_t len);

uint8_t M24C64_Write_Byte(uint16_t addr, uint8_t *buf, uint16_t len);
uint8_t M24C64_Read_Byte(uint16_t addr, uint8_t *buf, uint16_t len);
#endif
