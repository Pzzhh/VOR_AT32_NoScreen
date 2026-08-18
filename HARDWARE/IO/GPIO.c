/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-05-28 14:59:54
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 11:44:18
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\IO\GPIO.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-02-19 09:42:35
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-08-13 14:57:27
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\IO\GPIO.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "GPIO.h"
#include "at32f435_437.h"

const int tick_per_ms = 100; // 每毫秒的系统节拍数

static void Timer_init(int ms)
{
  crm_periph_clock_enable(CRM_TMR10_PERIPH_CLOCK, TRUE);

  TMR10->div = 28800;
  TMR10->pr = ms * tick_per_ms; // interval 10ms
  TMR10->cval = 1;
  TMR10->ctrl1_bit.tmren = TRUE;
  TMR10->iden_bit.ovfien = TRUE;
  TMR10->ists = 0;
  // nvic_priority_group_config(NVIC_PRIORITY_GROUP_2);
  nvic_irq_enable(TMR1_OVF_TMR10_IRQn, 0, 3);
  // GPIOF->ODR |= GPIO_ODR_ODR_15;
}
extern void HAL_USB_Log_Printf(const char *format, ...);
void CAM_GPIO_INIT()
{
  gpio_init_type gpio_init_struct;

  crm_periph_clock_enable(CRM_GPIOG_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOF_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

  gpio_init_struct.gpio_pins = GPIO_PINS_0;
  gpio_init(GPIOG, &gpio_init_struct); // REC

  gpio_init_struct.gpio_pins = GPIO_PINS_1;
  gpio_init(GPIOG, &gpio_init_struct); // WIFI

  gpio_init_struct.gpio_pins = GPIO_PINS_15;
  gpio_init(GPIOF, &gpio_init_struct); // SYNC LED

  gpio_init_struct.gpio_pins = GPIO_PINS_3;
  gpio_init(GPIOD, &gpio_init_struct); // BEEP

  gpio_init_struct.gpio_pins = GPIO_PINS_8;
  gpio_init_struct.gpio_pull = GPIO_PULL_DOWN; // 下拉开启ESP32
  gpio_init(GPIOC, &gpio_init_struct);

  gpio_bits_reset(GPIOG, GPIO_PINS_0);  // REC
  gpio_bits_reset(GPIOG, GPIO_PINS_1);  // WIFI
  gpio_bits_reset(GPIOF, GPIO_PINS_15); // SYNC LED
  gpio_bits_reset(GPIOD, GPIO_PINS_3);  // BEEP
  gpio_bits_reset(GPIOC, GPIO_PINS_8);  // ESP32
  Timer_init(10);
}

// IO 延时任务：到期时间为0表示无任务
static volatile uint32_t gpio_tick_ms = 0; // 中断自维护计数，每次中断+10ms
static uint32_t BeepOffTick = 0;           // Beep 到期关闭时间
static uint32_t SyncLEDOffTick = 0;        // SyncLED 到期关闭时间
static uint32_t WifiOffTick = 0;
static uint32_t BeepCount=0;

static uint32_t GPIO_GetTick(void)
{
  return gpio_tick_ms;
}

void TMR1_OVF_TMR10_IRQHandler()
{
  TMR10->ists = 0;
  gpio_tick_ms += tick_per_ms;
  if (gpio_tick_ms > 0xFFFF0000) // overflow
  {
    gpio_tick_ms = 0;
    BeepOffTick = 1;
    WifiOffTick = 1;
    SyncLEDOffTick = 1;
  }
  uint32_t now = gpio_tick_ms;

  if (BeepOffTick != 0 && now >= BeepOffTick)
  {
    gpio_bits_reset(GPIOD, GPIO_PINS_3); // 关闭 BEEP
    BeepOffTick = 0;
		//	HAL_USB_Log_Printf("beep %d ",WifiOffTick);
  }

  if (WifiOffTick != 0 && now >= WifiOffTick)
  {
    gpio_bits_reset(GPIOG, GPIO_PINS_0);
    WifiOffTick = 0;
		//HAL_USB_Log_Printf("wifi %d ",WifiOffTick);
  }

  if (SyncLEDOffTick != 0 && now >= SyncLEDOffTick)
  {
    gpio_bits_reset(GPIOF, GPIO_PINS_15); // 关闭 SyncLED
    SyncLEDOffTick = 0;
  }
	
}

void GPIO_CAM_REC_IO_Ctrl(int flag)
{
  if (flag)
    gpio_bits_set(GPIOG, GPIO_PINS_1);
  else
    gpio_bits_reset(GPIOG, GPIO_PINS_1);
}

void GPIO_CAM_SyncLED_IO_Ctrl()
{
  gpio_bits_set(GPIOF, GPIO_PINS_15);
  SyncLEDOffTick = GPIO_GetTick() + 5000; // 5秒后关闭
}

void GPIO_CAM_Wifi_IO_Ctrl()
{
  gpio_bits_set(GPIOG, GPIO_PINS_0);
  WifiOffTick = GPIO_GetTick() + 1000;
	//GPIO_Beep_IO_Ctrl(1000);
	//HAL_USB_Log_Printf("%d",WifiOffTick);
}

void GPIO_Beep_IO_Ctrl(uint32_t ms)
{
  gpio_bits_set(GPIOD, GPIO_PINS_3); // 打开 BEEP（低电平有效）
	BeepCount=3;
  BeepOffTick = GPIO_GetTick() + 500;   // ms 毫秒后由中断关闭
}