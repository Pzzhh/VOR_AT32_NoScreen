/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-30 14:37:06
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-09-22 22:19:24
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\HARDWARE\USART\USART5.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "USART3.h"
#include "at32f435_437.h"
#include "string.h"
#include "stdarg.h"
#include "stdio.h"

static void (*Interrupt_handle)(uint8_t dr);

void USART3_INIT(
    uint32_t boundrate,
    void (*handle)(uint8_t dr))
{
    gpio_init_type gpio_init_struct;

    /* enable the USART3 and gpio clock */
    crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);

    /* configure the USART3 tx, rx pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = GPIO_PINS_11 | GPIO_PINS_10;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(GPIOC, &gpio_init_struct);
    gpio_pin_mux_config(GPIOC, GPIO_PINS_SOURCE11, GPIO_MUX_7);
    gpio_pin_mux_config(GPIOC, GPIO_PINS_SOURCE10, GPIO_MUX_7);

    // nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    nvic_irq_enable(USART3_IRQn, 0, 0);

    /* configure USART3 param */
    usart_init(USART3, boundrate, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_parity_selection_config(USART3, USART_PARITY_NONE);
    usart_transmitter_enable(USART3, TRUE);
    usart_receiver_enable(USART3, TRUE);
    usart_enable(USART3, TRUE);

    usart_interrupt_enable(USART3, USART_RDBF_INT, TRUE);
    Interrupt_handle = handle;
}
static char printf_buf[100];
static uint16_t send_index, send_len;
uint8_t USART3_FIFO_Send(uint8_t *dr)
{
    if (send_index >= send_len)
    {
        send_index = 0;
        send_len = 0;
        return 0;
    }
    if (send_len)
    {
        *dr = printf_buf[send_index++];
    }
    return 1;
}

uint8_t USART3_Send_Package(char *string, uint16_t size)
{
    if (send_len)
        return 0;
    memcpy(printf_buf, string, size);
    send_len = size;
    usart_interrupt_enable(USART3, USART_TDBE_INT, TRUE); // 开启相关中断
    return 1;
}

uint8_t USART3_PRINTF(char *string, uint16_t size)
{
    if (send_len)
        return 0;
    memcpy(printf_buf, string, size);
    send_len = size;
    usart_interrupt_enable(USART3, USART_TDBE_INT, TRUE); // 开启相关中断
    return 1;
}

void USART3_IRQHandler(void) // 串口3中断服务程序
{
    if (usart_flag_get(USART3, USART_RDBF_FLAG) != RESET) // 接收中断(接收到的数据必须??0x0d 0x0a结尾)
    {
        uint8_t dr = usart_data_receive(USART3);
        if (Interrupt_handle != 0)
            Interrupt_handle(dr);
        usart_flag_clear(USART3, USART_RDBF_FLAG);
    }
    if (usart_flag_get(USART3, USART_TDBE_FLAG))
    {
        uint8_t dr;
        if (USART3_FIFO_Send(&dr))
        {
            usart_data_transmit(USART3, dr);
        }
        else
        {
            usart_interrupt_enable(USART3, USART_TDBE_INT, FALSE);
        }
    }
}