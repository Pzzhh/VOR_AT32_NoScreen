/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:48:47
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-15 11:52:46
 * @FilePath: \Slave_At32\src\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 **************************************************************************
 * @file     main.c
 * @brief    main program
 **************************************************************************
 *                       Copyright notice & Disclaimer
 *
 * The software Board Support Package (BSP) that is made available to
 * download from Artery official website is the copyrighted work of Artery.
 * Artery authorizes customers to use, copy, and distribute the BSP
 * software and its related documentation for the purpose of design and
 * development in conjunction with Artery microcontrollers. Use of the
 * software is governed by this copyright notice and the following disclaimer.
 * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
 * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
 * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
 * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
 * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
 *
 **************************************************************************
 */

// #include "at32f435_437_board.h"
#include "at32f435_437_clock.h"
#include "at32f435_437.h"
/** @addtogroup AT32F435_periph_examples
 * @{
 */

/** @addtogroup 435_TMR_7_pwm_output TMR_7_pwm_output
 * @{
 */

gpio_init_type gpio_init_struct = {0};
tmr_output_config_type tmr_output_struct;
crm_clocks_freq_type crm_clocks_freq_struct = {0};

uint16_t timerperiod = 0;
uint16_t channel1pulse = 0, channel2pulse = 0, channel3pulse = 0, channel4pulse = 0;

/**
 * @brief  main function.
 * @param  none
 * @retval none
 */
// int main(void)
//{
//   system_clock_config();

//}

/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 15:01:34
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-03-31 15:12:35
 * @FilePath: \USER\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "lcd.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
// #include "lv_conf.h"
#include "ft5206.h"
#include "gt9147.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lv_conf.h"
#include "../INTERFACE/UI/other/system_function.h"
#include "../other/meassage_center.h"
#include "../HARDWARE/Motor/inc_motor.h"
#include "../commuication/communication_API.h"  // USB 通信 API

volatile int idex = 0;
int *pt = (int *)0x20001000;
TaskHandle_t Thread_Lvgl;

void TIM3_Int_Init(u16 arr, u16 psc)
{
  crm_periph_clock_enable(CRM_TMR12_PERIPH_CLOCK, TRUE);

  TMR12->div = psc;
  TMR12->pr = arr; // interval 10ms
  TMR12->cval = 1;
  TMR12->ctrl1_bit.tmren = TRUE;
  TMR12->iden_bit.ovfien = TRUE;
  TMR12->ists = 0;
  // nvic_priority_group_config(NVIC_PRIORITY_GROUP_2);
  nvic_irq_enable(TMR8_BRK_TMR12_IRQn, 0, 3);
}

extern void Task_Stroage_Init();
extern void controlInit();
extern void mainpage_init(lv_obj_t *parent);
extern void UI_Page_Management_Init();

void LVGL_HANDLER()
{
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_2);
  FT5206_Init();
  GT9147_Init();
  LCD_Display_Dir(1);
  lv_init();            // lvgl绯荤粺鍒濆?嬶拷?
  lv_port_disp_init();  // lvgl鏄剧ず鎺ュ彛鍒濆?嬶拷?,鏀惧湪lv_init()鐨勫悗锟??
  lv_port_indev_init(); // lvgl杈撳叆鎺ュ彛鍒濆?嬶拷?,鏀惧湪lv_init()鐨勫悗锟??
  // 瀹氭椂鍣ㄥ垵濮嬪寲(1ms涓?鏂?),鐢ㄤ簬缁檒vgl鎻愪緵1ms鐨勫績璺宠妭锟??
  /*system init */
  controlInit();
  uint8_t buf[20] = {0};
  int isReady = 0;
  ControlDelay(100);
	extern void SRV1_Encoder_Init();
	 SRV1_Encoder_Init();
  while (1)
  {
    Message_Center_Read_prinft("Ctrl", buf, 20, "ISCtrlInit");
    if (sscanf(buf, "ISCtrlInit %d", &isReady) == 1)
    {
      if (isReady)
      {
        break;
      }
    }
    ControlDelay(500);
  }
	
  Task_Stroage_Init();
  UI_Page_Management_Init();
  uint32_t touch_tick=0;
  while (1)
  {
    // GT9147_Scan(0);
    if (touch_tick + 20 < ControlGetTick())
    {
      touch_sever(1, NULL);
      touch_tick = ControlGetTick();
    }
    
    lv_task_handler();
    ControlDelay(2);
  }
}

void get_pt()
{

  *pt = *pt + 1;
}
#include "../HARDWARE/EROM/eriic.h"
uint32_t value = 0;
uint8_t isDebuggerConnected = 0;
int main(void)
{
  SCB->VTOR = 0x08001000;
  isDebuggerConnected = *((uint32_t *)0xE000EDF0) & 1;
  system_clock_config();
  TIM3_Int_Init(19999, 288 - 1);
  delay_init();

  if (value)
  {
    crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, false);
  }

  // ...existing code...
  DFU_PowerUp_Detection();
		delay_ms(1000);
  LCD_Init();
	delay_ms(1000);
  //    WWDG_Init(0x7F, 0x5F, WWDG_Prescaler_8); // 初始化窗口看门狗，举例参数
  TaskHandle_t handle;
  xTaskCreate((TaskFunction_t)LVGL_HANDLER,
              (const char *)"Outside_motor",
              (uint16_t)8000,
              (void *)NULL,
              (UBaseType_t)15,
              (TaskHandle_t *)&handle);
  vTaskStartScheduler();
}

// 定时器3中断服务程序
void TMR8_BRK_TMR12_IRQHandler(void)
{
  if (TMR12->ists_bit.ovfif) // 溢出中断
  {
    if (isDebuggerConnected == 0)
      wdt_counter_reload();
  }
  TMR12->ists = 0;
}

/**
 * @}
 */

/**
 * @}
 */
