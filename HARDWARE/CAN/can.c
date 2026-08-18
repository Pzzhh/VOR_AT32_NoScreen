/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-13 11:18:33
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-23 10:05:12
 * @FilePath: \USERd:\workfile\项目3 vor\software\vor-pjt\template\HARDWARE\CAN\can.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 ****************************************************************************************************
 * @file        can.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-01-11
 * @brief       CAN 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20220111
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "../HARDWARE/CAN/can.h"
#include "at32f435_437.h"
void (*can_rx_handler)(can_rx_message_type *msg);

/* extended identifier */
#define FILTER_EXT_ID1 ((uint32_t)0x18F5F100)
#define FILTER_EXT_ID2 ((uint32_t)0x18F5F200)
#define FILTER_EXT_ID3 ((uint32_t)0x18F5F300)

/* standard identifier */
#define FILTER_STD_ID1 ((uint16_t)0x04F6)
#define FILTER_STD_ID2 ((uint16_t)0x04F7)
#define FILTER_STD_ID3 ((uint16_t)0x04F8)

static void can_gpio_config(void)
{
  gpio_init_type gpio_init_struct;
  /* enable the gpio clock */
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

  gpio_default_para_init(&gpio_init_struct);

  /* configure the can tx, rx pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_12 | GPIO_PINS_13;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOB, &gpio_init_struct);

  gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE12, GPIO_MUX_9);
  gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE13, GPIO_MUX_9);
}



/**
 *  @brief  can configiguration.
 *  @param  none
 *  @retval none
 */
void CAN2_INIT(void)
{
  can_base_type can_base_struct;
  can_baudrate_type can_baudrate_struct;
  can_filter_init_type can_filter_init_struct;
  /* enable the can clock */
  crm_periph_clock_enable(CRM_CAN2_PERIPH_CLOCK, TRUE);

  /* can base init */
  can_default_para_init(&can_base_struct);
  can_gpio_config();
  can_base_struct.mode_selection = CAN_MODE_COMMUNICATE;
  can_base_struct.ttc_enable = FALSE;
  can_base_struct.aebo_enable = TRUE;
  can_base_struct.aed_enable = TRUE;
  can_base_struct.prsf_enable = FALSE;
  can_base_struct.mdrsel_selection = CAN_DISCARDING_FIRST_RECEIVED;
  can_base_struct.mmssr_selection = CAN_SENDING_BY_ID;
  can_base_init(CAN2, &can_base_struct);

  /* can baudrate, set boudrate = pclk/(baudrate_div *(1 + bts1_size + bts2_size)) */
  can_baudrate_struct.baudrate_div = 12;
  can_baudrate_struct.rsaw_size = CAN_RSAW_3TQ;
  can_baudrate_struct.bts1_size = CAN_BTS1_3TQ;
  can_baudrate_struct.bts2_size = CAN_BTS2_8TQ;
  can_baudrate_set(CAN2, &can_baudrate_struct);

  /* can filter 0 config */
  can_filter_init_struct.filter_activate_enable = TRUE;
  can_filter_init_struct.filter_mode = CAN_FILTER_MODE_ID_MASK;
  can_filter_init_struct.filter_fifo = CAN_FILTER_FIFO0;
  can_filter_init_struct.filter_number = 0;
  can_filter_init_struct.filter_bit = CAN_FILTER_32BIT;
  can_filter_init_struct.filter_id_high =0; /* extended identifier is 29 bit */
  can_filter_init_struct.filter_id_low = 0;
  can_filter_init_struct.filter_mask_high = 0; /* extended identifier is 29 bit */
  can_filter_init_struct.filter_mask_low = 0;
  can_filter_init(CAN2, &can_filter_init_struct);


  /* can interrupt config */
  nvic_irq_enable(CAN2_SE_IRQn, 0x00, 0x00);
  nvic_irq_enable(CAN2_RX0_IRQn, 0x00, 0x00);
  can_interrupt_enable(CAN2, CAN_RF0MIEN_INT, TRUE);

  /* error interrupt enable */
  can_interrupt_enable(CAN2, CAN_ETRIEN_INT, TRUE);
  can_interrupt_enable(CAN2, CAN_EOIEN_INT, TRUE);
}

uint8_t can_init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, uint16_t brp, uint8_t mode) /* CAN初始化 */
{
  CAN2_INIT();
}

/**
 *  @brief  can transmit data
 *  @param  none
 *  @retval none
 */
#define FILTER_STD_ID1 ((uint16_t)0x04F6)
int can_transmit_data(uint16_t STD_ID, uint8_t *data, uint8_t len)
{
  uint8_t transmit_mailbox;
  can_tx_message_type tx_message_struct;
  if (len > 8)
    return 0;

  /* transmit FILTER_STD_ID1 */
  tx_message_struct.standard_id = STD_ID;
  tx_message_struct.extended_id = 0;
  tx_message_struct.id_type = CAN_ID_STANDARD;
  tx_message_struct.frame_type = CAN_TFT_DATA;
  tx_message_struct.dlc = len;
  for (int i = 0; i < len; i++)
  {
    tx_message_struct.data[i] = data[i];
  }

  transmit_mailbox = can_message_transmit(CAN2, &tx_message_struct);
  return (can_transmit_status_get(CAN2, (can_tx_mailbox_num_type)transmit_mailbox) != CAN_TX_STATUS_SUCCESSFUL);
}

/**
 *  @brief  CAN2 interrupt function rx0
 *  @param  none
 *  @retval none
 */
void CAN2_RX0_IRQHandler(void)
{
  can_rx_message_type rx_message_struct;
  if (can_interrupt_flag_get(CAN2, CAN_RF0MN_FLAG) != RESET)
  {
    can_message_receive(CAN2, CAN_RX_FIFO0, &rx_message_struct);
    if (can_rx_handler != 0)
      can_rx_handler(&rx_message_struct);
  }
}

/**
 *  @brief  CAN2 interrupt function se
 *  @param  none
 *  @retval none
 */
void CAN2_SE_IRQHandler(void)
{
  __IO uint32_t err_index = 0;
  if (can_interrupt_flag_get(CAN2, CAN_ETR_FLAG) != RESET)
  {
    err_index = CAN2->ests & 0x70;
    can_flag_clear(CAN2, CAN_ETR_FLAG);
    /* error type is stuff error */
    if (err_index == 0x00000010)
    {
      /* when stuff error occur: in order to ensure communication normally,
      user must restart can or send a frame of highest priority message here */
    }
  }
}

/**
 * @brief       CAN发送一条消息
 * @param       id      : 标准ID(11位)/扩展ID(11位+18位)
 * @param       ide     : 0,标准帧;1,扩展帧
 * @param       rtr     : 0,数据帧;1,远程帧
 * @param       len     : 要发送的数据长度(固定为8个字节, 在时间触发模式下, 有效数据为6个字节)
 * @param       data    : 数据指针
 * @retval      本次发送数据所用的邮箱号
 *   @arg       0 ~ 3   : 邮箱编号
 *   @arg       0XFF    : 无有效邮箱(数据发送失败)
 */
uint8_t can_tx_msg(uint32_t id, uint8_t ide, uint8_t rtr, uint8_t len, uint8_t *data)
{
  uint8_t res;
  if( can_transmit_data(id, data, len))
    res = 0xFF;
  else
    res = 0;
  return res;
}

/**
 * @brief       CAN 发送一组数据
 *   @note      发送格式固定为: 标准ID, 数据帧
 * @param       id      : 标准ID(11位)
 * @retval      发送状态 0, 成功; 1, 失败;
 */
uint8_t can_send_msg(uint32_t id, uint8_t *msg, uint8_t len)
{
  uint8_t res = 10;
   res=can_transmit_data(id, msg, len);
  return (res) ? 0 : 1;
}
