/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-04-21 19:16:22
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-05-29 15:58:00
 * @FilePath: \P0_Primary_PJ\usb\usb_init.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __USB_INIT_H_
#define __USB_INIT_H_

#include "at32f435_437.h"
#include "delay.h"


void USB1_Init(void);
void usb_send(uint8_t *report, uint16_t len);
uint8_t USB_CDC_Send(uint8_t CH, uint8_t *buf, uint16_t len);
uint8_t USB_CDC_Send_Force(uint8_t CH, uint8_t *buf, uint16_t len);
uint8_t USB_CDC_Tx_Completed(uint8_t CH);
uint8_t USB_CDC_Rx_Completed(uint8_t CH);
uint8_t USB_CDC_Is_Initialized(void);
uint16_t USB_CDC_GET(uint8_t CH, uint8_t *buf);
#endif
