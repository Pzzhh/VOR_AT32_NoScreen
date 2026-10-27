/**
  **************************************************************************
  * @file     cdc_class.h
  * @brief    usb cdc class file
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

 /* define to prevent recursive inclusion -------------------------------------*/
#ifndef __CDC_CLASS_H
#define __CDC_CLASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_std.h"
#include "usbd_core.h"

/** @addtogroup AT32F435_437_middlewares_usbd_class
  * @{
  */

/** @addtogroup USB_cdc_class
  * @{
  */

/** @defgroup USB_cdc_class_definition
  * @{
  */

/**
  * @brief usb cdc use endpoint define
  */
#define USBD_CDC_INT_EPT                 0x82
#define USBD_CDC_BULK_IN_EPT             0x81
#define USBD_CDC_BULK_OUT_EPT            0x01
#define USBD_CDC2_INT_EPT                 0x84
#define USBD_CDC2_BULK_IN_EPT             0x83
#define USBD_CDC2_BULK_OUT_EPT            0x03

/**
  * @brief usb cdc in and out max packet size define
  */
#define USBD_CDC_IN_MAXPACKET_SIZE        0x40
#define USBD_CDC_OUT_MAXPACKET_SIZE       0x40
#define USBD_CDC_CMD_MAXPACKET_SIZE       0x08

/**
  * @}
  */

/** @defgroup USB_cdc_class_exported_types
  * @{
  */

/**
  * @brief usb cdc class struct
  */
typedef struct
{
  uint32_t alt_setting[2];
  uint8_t g_rx_buff[2][USBD_CDC_OUT_MAXPACKET_SIZE];
  uint8_t g_cmd[2][USBD_CDC_CMD_MAXPACKET_SIZE];
  uint8_t g_req[2];
  uint8_t control_port;
  uint16_t g_len[2], g_rxlen[2];
  __IO uint8_t g_tx_completed[2], g_rx_completed[2];
  linecoding_type linecoding[2];
}cdc_struct_type;


/**
  * @}
  */

/** @defgroup USB_cdc_class_exported_functions
  * @{
  */
extern usbd_class_handler cdc_class_handler;
uint16_t usb_vcp_get_rxdata(void *udev, uint8_t *recv_data);
error_status usb_vcp_send_data(void *udev, uint8_t *send_data, uint16_t len);
error_status usb_vcp_send_data_force(void *udev, uint8_t *send_data, uint16_t len);
uint8_t usb_vcp_tx_completed(void *udev);
uint8_t usb_vcp_rx_completed(void *udev);
uint16_t usb_vcp_get_rxdata_ch(void *udev, uint8_t channel, uint8_t *recv_data);
error_status usb_vcp_send_data_ch(void *udev, uint8_t channel, uint8_t *send_data, uint16_t len);
uint8_t usb_vcp_tx_completed_ch(void *udev, uint8_t channel);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif




