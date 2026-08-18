#include "eriic.h"
#include "delay.h"
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
// 控制I2C速度的延时

typedef struct
{
	uint16_t *set_bsr;
	uint16_t *clr_bsr;
	uint32_t *read_reg;
	uint32_t scl_mask;
	uint32_t sda_mask;
	// void (*delay)(uint32_t );
} m_iic_ops;

void ER_Delay_ms(uint16_t ms)
{
	for (int i = 0; i < ms; i++)
	{
		const uint32_t nus = 1000;
		TMR11->ctrl1_bit.tmren = 0;
		TMR11->ctrl1 = 0;
		TMR11->div = (144) - 1;
		TMR11->pr = (nus);
			__DSB();
		TMR11->cval = 0;
		TMR11->ists = 0;
		TMR11->ctrl1_bit.tmren = 1;
		while (TMR11->ists_bit.ovfif == 0)
		{
		}
		return;
	}
}

void ER_Delay(void)
{
	const uint32_t nus = 2;
	TMR11->ctrl1_bit.tmren = 0;
	TMR11->ctrl1 = 0;
	TMR11->div = (288) - 1;
	TMR11->pr = (nus);
		__DSB();
	TMR11->cval = 0;
	TMR11->ists = 0;
	TMR11->ctrl1_bit.tmren = 1;
	while (TMR11->ists_bit.ovfif == 0)
	{
	}
	return;
}
// 电容触摸芯片IIC接口初始化
void ER_IIC_Init(void)
{
	gpio_init_type gpio_init_struct;
	CRM->apb2en_bit.tmr11en = 1;
	crm_periph_clock_enable(CRM_GPIOG_PERIPH_CLOCK, TRUE);

	gpio_default_para_init(&gpio_init_struct);
	// PB0 as output pull-up (SCL)
	gpio_init_struct.gpio_pins = GPIO_PINS_7;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
	gpio_init_struct.gpio_pull = GPIO_PULL_UP;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
	gpio_init(GPIOG, &gpio_init_struct);

	// PF11 as output pull-up (SDA)
	gpio_init_struct.gpio_pins = GPIO_PINS_8;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
	gpio_init_struct.gpio_pull = GPIO_PULL_UP;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
	gpio_init(GPIOG, &gpio_init_struct);
}
// 产生IIC起始信号
void ER_IIC_Start(void)
{
	ER_IIC_SDA = 1;
	ER_IIC_SCL = 1;
	delay_us(2);
	ER_IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
	ER_Delay();
	ER_IIC_SCL = 0; // 钳住I2C总线，准备发送或接收数据
}
// 产生IIC停止信号
void ER_IIC_Stop(void)
{
	ER_IIC_SDA = 0;
	ER_Delay();
	ER_IIC_SCL = 1;
	delay_us(2);
	ER_IIC_SDA = 1; // 发送I2C总线结束信号
	ER_Delay();
}
// 等待应答信号到来
// 返回值：1，接收应答失败
//         0，接收应答成功
u8 ER_IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	ER_IIC_SDA = 1;
	ER_Delay();
	ER_IIC_SCL = 1;
	ER_Delay();
	while (ER_READ_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			ER_IIC_Stop();
			return 1;
		}
		ER_Delay();
	}
	ER_IIC_SCL = 0; // 时钟输出0
	return 0;
}
// 产生ACK应答
void ER_IIC_Ack(void)
{
	ER_IIC_SCL = 0;

	ER_Delay();
	ER_IIC_SDA = 0;
	ER_Delay();
	ER_IIC_SCL = 1;
	ER_Delay();
	ER_IIC_SCL = 0;
	ER_IIC_SDA = 1;
}
// 不产生ACK应答
void ER_IIC_NAck(void)
{
	ER_IIC_SCL = 0;
	ER_Delay();
	ER_IIC_SDA = 1;
	ER_Delay();
	ER_IIC_SCL = 1;
	ER_Delay();
	ER_IIC_SCL = 0;
}
// IIC发送一个字节
// 返回从机有无应答
// 1，有应答
// 0，无应答
void ER_IIC_Send_Byte(u8 txd)
{
	u8 t;
	ER_IIC_SCL = 0; // 拉低时钟开始数据传输
	ER_Delay();
	for (t = 0; t < 8; t++)
	{
		ER_IIC_SDA = (txd & 0x80) >> 7;
		txd <<= 1;
		ER_Delay();
		ER_IIC_SCL = 1;
		ER_Delay();
		ER_IIC_SCL = 0;
		ER_Delay();
	}
	ER_IIC_SDA = 1;
}
// 读1个字节，ack=1时，发送ACK，ack=0，发送nACK
u8 ER_IIC_Read_one_Bytes(unsigned char ack)
{
	u8 i, receive = 0;
	delay_us(5);
	for (i = 0; i < 8; i++)
	{
		ER_IIC_SCL = 0;
		ER_Delay();
		ER_IIC_SCL = 1;
		ER_Delay();
		receive <<= 1;
		if (ER_READ_SDA)
			receive++;
	}
	ER_IIC_SCL = 0;
	if (!ack)
		ER_IIC_NAck(); // 发送nACK
	else
		ER_IIC_Ack(); // 发送ACK
	return receive;
}

volatile uint16_t lenread=0;
uint8_t ER_IIC_Read_Bytes(uint8_t I2C_Addr, uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i, res = 0;
	ER_IIC_Start();
	ER_IIC_Send_Byte(I2C_Addr); // 发送器件地址+写信号
	if (ER_IIC_Wait_Ack())
	{
		ER_IIC_Stop();
		return 1;
	}
	ER_IIC_Send_Byte(addr >> 8); // 发送寄存器地址
	ER_IIC_Wait_Ack();
	ER_IIC_Send_Byte(addr & 0xFF); // 发送寄存器地址
	ER_IIC_Wait_Ack();
	ER_IIC_Start();
	ER_IIC_Send_Byte(I2C_Addr + 1); // 发送器件地址+读信号
	ER_IIC_Wait_Ack();
	for (i = 0; i < len; i++)
	{
		lenread=1;
		if (i != len - 1)
			*buf++ = ER_IIC_Read_one_Bytes(1); // 带ACK
		else
			*buf++ = ER_IIC_Read_one_Bytes(0); // 最后一个字节NACK
	}
	ER_IIC_Stop(); // 产生一个停止条件
	return 0;
}

uint8_t ER_IIC_Write_Bytes(uint8_t I2C_Addr, uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i;
	ER_IIC_Start();
	ER_IIC_Send_Byte(I2C_Addr); // 发送器件地址+写信号
	if (ER_IIC_Wait_Ack())
	{
		ER_IIC_Stop();
		return 1;
	}
	ER_IIC_Send_Byte(addr >> 8); // 发送寄存器地址
	ER_IIC_Wait_Ack();
	ER_IIC_Send_Byte(addr & 0xFF); // 发送寄存器地址
	ER_IIC_Wait_Ack();
	for (i = 0; i < len; i++)
	{
		ER_IIC_Send_Byte(*buf++); // 发送数据
		if (ER_IIC_Wait_Ack())
		{
			ER_IIC_Stop();
			return 1;
		}
	}
	ER_IIC_Stop(); // 产生一个停止条件
	return 0;
}
#include "string.h"
uint8_t M24C64_Write_Byte(uint16_t addr, uint8_t *buf, uint16_t len)
{
	const uint16_t pagemask = ~0x1f;
	const uint8_t m24_addr = 0xA0;
	uint8_t cache[32] = 0;
	uint8_t pagelen = 0;
	uint16_t pageWC = len / 32 + (addr % 32 ? 1 : 0);
	// uint16_t bufId = 0;
	uint16_t fpid = addr % 32; // first page id
	uint16_t fpNw = 32 - fpid; // first page need write count
	fpNw = (len < fpNw) ? len : fpNw;
	uint8_t wait = 20;
	ER_IIC_Read_Bytes(m24_addr, addr & pagemask, (uint8_t *)cache, 32);
	for (int i = 0; i < fpNw; i++)
	{
		cache[i + fpid] = buf[i];
	}
	for (int i = 0; i < pageWC; i++)
	{
		if (i == 0)
			pagelen = 32;
		else
			pagelen = (i == pageWC - 1) ? addr % 32 : 32;
		if (i != 0)
			memcpy(cache, &buf[i * 32 + fpNw - 32], pagelen);
		while (wait--)
		{
			uint8_t res = ER_IIC_Write_Bytes(m24_addr, (addr + i * 32) & pagemask,
											 cache, pagelen);
			if (res == 0)
				break;
			ER_Delay_ms(1);
		}
	}
	//		ER_Delay_ms(10);
	return 0;
}

uint8_t M24C64_Read_Byte(uint16_t addr, uint8_t *buf, uint16_t len)
{
	const uint16_t pagemask = ~0x1f;
	const uint8_t m24_addr = 0xA0;
	uint8_t wait = 20;
	while (wait--)
	{
		uint8_t res = ER_IIC_Read_Bytes(m24_addr, addr, (uint8_t *)buf, len);
		if (res == 0)
			break;
		ER_Delay_ms(1);
	}
	return 0;
}
