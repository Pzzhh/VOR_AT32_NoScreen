#include "delay.h"
#include "at32f435_437.h"
//////////////////////////////////////////////////////////////////////////////////
// 如果使用OS,则包括下面的头文件（以ucos为例）即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h" //支持OS时，使用
#endif
//////////////////////////////////////////////////////////////////////////////////
// 本程序只供学习使用，未经作者许可，不得用于其它任何用途
// ALIENTEK STM32F407开发板
// 使用SysTick的普通计数模式对延迟进行管理(支持OS)
// 包括delay_us,delay_ms
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/2
// 版本：V1.3
// 版权所有，盗版必究。
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//********************************************************************************
// 修改说明
// V1.1 20140803
// 1,delay_us,添加参数等于0判断,如果参数等于0,则直接退出.
// 2,修改ucosii下,delay_ms函数,加入OSLockNesting的判断,在进入中断后,也可以准确延时.
// V1.2 20150411
// 修改OS支持方式,以支持任意OS(不限于UCOSII和UCOSIII,理论上任意OS都可以支持)
// 添加:delay_osrunning/delay_ostickspersec/delay_osintnesting三个宏定义
// 添加:delay_osschedlock/delay_osschedunlock/delay_ostimedly三个函数
// V1.3 20150521
// 修正UCOSIII支持时的2个bug：
// delay_tickspersec改为：delay_ostickspersec
// delay_intnesting改为：delay_osintnesting
//////////////////////////////////////////////////////////////////////////////////

// static u8 fac_us = 0;  // us延时倍乘数
// static u16 fac_ms = 0; // ms延时倍乘数,在os下,代表每个节拍的ms数

#if SYSTEM_SUPPORT_OS // 如果SYSTEM_SUPPORT_OS定义了,说明要支持OS了(不限于UCOS).
// 当delay_us/delay_ms需要支持OS的时候需要三个与OS相关的宏定义和函数来支持
// 首先是3个宏定义:
//     delay_osrunning:用于表示OS当前是否正在运行,以决定是否可以使用相关函数
// delay_ostickspersec:用于表示OS设定的时钟节拍,delay_init将根据这个参数来初始哈systick
//  delay_osintnesting:用于表示OS中断嵌套级别,因为中断里面不可以调度,delay_ms使用该参数来决定如何运行
// 然后是3个函数:
//   delay_osschedlock:用于锁定OS任务调度,禁止调度
// delay_osschedunlock:用于解锁OS任务调度,重新开启调度
//     delay_ostimedly:用于OS延时,可以引起任务调度.

// 本例程仅作UCOSII和UCOSIII的支持,其他OS,请自行参考着移植
// 支持UCOSII
#ifdef OS_CRITICAL_METHOD					 // OS_CRITICAL_METHOD定义了,说明要支持UCOSII
#define delay_osrunning OSRunning			 // OS是否运行标记,0,不运行;1,在运行
#define delay_ostickspersec OS_TICKS_PER_SEC // OS时钟节拍,即每秒调度次数
#define delay_osintnesting OSIntNesting		 // 中断嵌套级别,即中断嵌套次数
#endif

// 支持UCOSIII
#ifdef CPU_CFG_CRITICAL_METHOD				  // CPU_CFG_CRITICAL_METHOD定义了,说明要支持UCOSIII
#define delay_osrunning OSRunning			  // OS是否运行标记,0,不运行;1,在运行
#define delay_ostickspersec OSCfg_TickRate_Hz // OS时钟节拍,即每秒调度次数
#define delay_osintnesting OSIntNestingCtr	  // 中断嵌套级别,即中断嵌套次数
#endif

// us级延时时,关闭任务调度(防止打断us级延迟)
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD // 使用UCOSIII
	OS_ERR err;
	OSSchedLock(&err); // UCOSIII的方式,禁止调度，防止打断us延时
#else				   // 否则UCOSII
	OSSchedLock(); // UCOSII的方式,禁止调度，防止打断us延时
#endif
}

// us级延时时,恢复任务调度
void delay_osschedunlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD // 使用UCOSIII
	OS_ERR err;
	OSSchedUnlock(&err); // UCOSIII的方式,恢复调度
#else					 // 否则UCOSII
	OSSchedUnlock(); // UCOSII的方式,恢复调度
#endif
}

// 调用OS自带的延时函数延时
// ticks:延时的节拍数
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err;
	OSTimeDly(ticks, OS_OPT_TIME_PERIODIC, &err); // UCOSIII延时采用周期模式
#else
	OSTimeDly(ticks); // UCOSII延时
#endif
}

// systick中断服务函数,使用OS时用到
void SysTick_Handler(void)
{
	if (delay_osrunning == 1) // OS开始跑了,才执行正常的调度处理
	{
		OSIntEnter(); // 进入中断
		OSTimeTick(); // 调用ucos的时钟服务程序
		OSIntExit();  // 触发任务切换软中断
	}
}
#endif

// 初始化延迟函数
// 当使用OS的时候,此函数会初始化OS的时钟节拍
// SYSTICK的时钟固定为AHB时钟的1/8
// SYSCLK:系统时钟频率
void delay_init(u8 SYSCLK)
{

	CRM->apb1en_bit.adc14en = 1;
	TMR14->ctrl1_bit.ocmen = 1;
//	CRM->ahben1_bit.gpioden = 1;
//	GPIOD->cfgr_bit.iomc3 = 1;
//	delay_us(10);
}

#if SYSTEM_SUPPORT_OS // 如果需要支持OS.
// 延时nus
// nus:要延时的us数.
// nus:0~204522252(最大值即2^32/fac_us@fac_us=21)
void delay_us(u32 nus)
{
	u32 ticks;
	u32 told, tnow, tcnt = 0;
	u32 reload = SysTick->LOAD; // LOAD的值
	ticks = nus * fac_us;		// 需要的节拍数
	delay_osschedlock();		// 阻止OS调度，防止打断us延时
	told = SysTick->VAL;		// 刚进入时的计数器值
	while (1)
	{
		tnow = SysTick->VAL;
		if (tnow != told)
		{
			if (tnow < told)
				tcnt += told - tnow; // 这里注意一下SYSTICK是一个递减的计数器就可以了.
			else
				tcnt += reload - tnow + told;
			told = tnow;
			if (tcnt >= ticks)
				break; // 时间超过/等于要延迟的时间,则退出.
		}
	};
	delay_osschedunlock(); // 恢复OS调度
}
// 延时nms
// nms:要延时的ms数
// nms:0~65535
void delay_ms(u16 nms)
{
	if (delay_osrunning && delay_osintnesting == 0) // 如果OS已经在跑了,并且不是在中断里面(中断里面不能任务调度)
	{
		if (nms >= fac_ms) // 延时的时间大于OS的最少时间周期
		{
			delay_ostimedly(nms / fac_ms); // OS延时
		}
		nms %= fac_ms; // OS已经无法提供这么小的延时了,采用普通方式延时
	}
	delay_us((u32)(nms * 1000)); // 普通方式延时
}
#else // 不用ucos时
// 延时nus
// nus为要延时的us数.
// 注意:nus的值,不要大于798915us(最大值即2^24/fac_us@fac_us=21)
void delay_us(u32 nus)
{
	// const uint32_t SystemClock = 168000000;
	// if (nus > 1000)
	// 	nus = 1000;
	// TIM14->CR1 = 0;
	// TIM14->PSC = (168 / 2) - 1;
	// TIM14->ARR = (10000) - 1;
	// TIM14->CNT = 0;
	// TIM14->CR1 |= (TIM_CR1_CEN | TIM_CR1_OPM);
	// while (TIM14->CNT < nus)
	// {
	// }
	// return;
	// uint32_t temp = 1 | TIM_CR1_OPM;
//	GPIOD->odt_bit.odt3 = 1;
	TMR14->ctrl1_bit.tmren = 0;
	if (nus > 1000)
		nus = 5000;
	if (nus == 0)
	{
		TMR14->ctrl1 = 0;
		TMR14->div = (288) - 1;
		TMR14->pr = 10;
		TMR14->cval = 0;
		__DSB();
		TMR14->ists = 0;
		TMR14->ctrl1_bit.tmren = 1;
		while (TMR14->ists_bit.ovfif == 0)
		{
		}
	}
	else
	{
		TMR14->ctrl1 = 0;
		TMR14->div = (288) - 1;
		TMR14->pr = (nus);
		TMR14->cval = 0;
		__DSB();
		TMR14->ists = 0;
		TMR14->ctrl1_bit.tmren = 1;
		while (TMR14->ists_bit.ovfif == 0)
		{
		}
	}
//	GPIOD->odt_bit.odt3 = 0;
	return;
}
// 延时nms
// 注意nms的范围
// SysTick->LOAD为24位寄存器,所以,最大延时为:
// nms<=0xffffff*8*1000/SYSCLK
// SYSCLK单位为Hz,nms单位为ms
// 对168M条件下,nms<=798ms
void delay_xms(u16 nms)
{
	// u32 temp;
	// SysTick->LOAD = (u32)nms * fac_ms;		  // 时间加载(SysTick->LOAD为24bit)
	// SysTick->VAL = 0x00;					  // 清空计数器
	// SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 开始倒数
	// do
	// {
	// 	temp = SysTick->CTRL;
	// } while ((temp & 0x01) && !(temp & (1 << 16))); // 等待时间到达
	// SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
	// SysTick->VAL = 0X00;					   // 清空计数器
}
int xx = 0;
// 延时nms
// nms:0~65535
void delay_ms(u16 nms)
{
	int i=0;
	for ( i = 0; i < nms; i++)
	{
		delay_us(1000);
	}
	
	return;
}

void jump_to_bootloader(void)
{
	// 禁用所有中断
	__disable_irq();

	// 设置 Bootloader 地址
	uint32_t bootloader_address = 0x1FFF0000;

	// 定义一个指向函数的指针，指向 Bootloader 的复位向量
	void (*bootloader_entry)(void) = (void (*)(void))(*((uint32_t *)(bootloader_address + 4)));

	// 设置主堆栈指针
	CRM->ahbrst2_bit.otgfs1rst = 1;
	CRM->ahbrst2_bit.otgfs1rst = 0;

	__set_MSP(*((uint32_t *)bootloader_address));

	// 跳转到 Bootloader
	bootloader_entry();
}

#endif
