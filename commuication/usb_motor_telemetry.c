#include "usb_motor_telemetry.h"
#include "USB/usb_init.h"
#include "../INTERFACE/UI/control/control.h"
#include "../INTERFACE/UI/control/control_Hardware_API.h"
#include "../INTERFACE/UI/other/system_function.h"
#include "../HARDWARE/c610/C610.h"
#include "at32f435_437.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <string.h>

#define TELEMETRY_CHANNEL       2U
#define TELEMETRY_SAMPLES       5U
#define TELEMETRY_HEADER_SIZE   2U
#define TELEMETRY_SAMPLE_SIZE   5U
#define TELEMETRY_PACKET_SIZE   (TELEMETRY_HEADER_SIZE + 2U + TELEMETRY_SAMPLES * TELEMETRY_SAMPLE_SIZE)
#define TELEMETRY_HEADER_0      0xAAU
#define TELEMETRY_HEADER_1      0x56U /* two channels + flash state */

extern Task_control_info control_info;

static int16_t Scale_To_I16(float value)
{
    if (value >= 32767.0f)
        return 32767;
    if (value <= -32768.0f)
        return (int16_t)-32768;
    return (int16_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void Put_I16_LE(uint8_t *dst, int16_t value)
{
    uint16_t raw = (uint16_t)value;
    dst[0] = (uint8_t)raw;
    dst[1] = (uint8_t)(raw >> 8);
}

void USB_Motor_Telemetry_Thread(void *argument)
{
    TickType_t wake_tick = xTaskGetTickCount();
    uint8_t packet[TELEMETRY_PACKET_SIZE];
    uint8_t tx_packet[TELEMETRY_PACKET_SIZE];
    uint8_t sample_count = 0;
    uint8_t phase = 0;
    uint8_t was_running = 0;
    uint16_t sequence = 0;
    uint32_t previous_ms = 0;
    float previous_angle = 0.0f;

    (void)argument;
    for (;;) {
        TickType_t period = (phase == 2U) ? pdMS_TO_TICKS(16U) : pdMS_TO_TICKS(17U);
        phase = (uint8_t)((phase + 1U) % 3U);
        vTaskDelayUntil(&wake_tick, period);

        if (!control_info.State_Bit.IsRunning || control_info.State_Bit.isPause ||
            control_info.taskArray == 0 ||
            control_info.currentCount >= control_info.taskCount ||
            control_info.taskArray[control_info.currentCount].mode == Task_Incline) {
            sample_count = 0;
            was_running = 0;
            phase = 0;
            continue;
        }

        {
            float angle = 0.0f;
            float speed = 0.0f;
            float can_angle = 0.0f;
            float can_speed = 0.0f;
            uint32_t now_ms = ControlGetTick();
            uint8_t offset;

            if (!HAL_Encode_GET_angle(&angle))
                continue;
            (void)C610_Get_Feedback(&can_angle, &can_speed);
            if (!was_running) {
                sequence = 0;
                sample_count = 0;
                previous_angle = angle;
                previous_ms = now_ms;
                was_running = 1;
            } else {
                uint32_t elapsed_ms = now_ms - previous_ms;
                if (elapsed_ms != 0U)
                    speed = (angle - previous_angle) * 1000.0f / (float)elapsed_ms;
            }

            if (sample_count == 0U) {
                packet[0] = TELEMETRY_HEADER_0;
                packet[1] = TELEMETRY_HEADER_1;
                packet[2] = (uint8_t)sequence;
                packet[3] = (uint8_t)(sequence >> 8);
            }
            offset = (uint8_t)(4U + sample_count * TELEMETRY_SAMPLE_SIZE);
            Put_I16_LE(&packet[offset], Scale_To_I16(speed * 10.0f));
            Put_I16_LE(&packet[offset + 2U], Scale_To_I16(can_angle * 10.0f));
            packet[offset + 4U] =
                gpio_output_data_bit_read(GPIOF, GPIO_PINS_15) == SET ? 1U : 0U;
            previous_angle = angle;
            previous_ms = now_ms;
            sample_count++;

            if (sample_count == TELEMETRY_SAMPLES) {
                if (USB_CDC_Tx_Completed(TELEMETRY_CHANNEL)) {
                    memcpy(tx_packet, packet, TELEMETRY_PACKET_SIZE);
                    (void)USB_CDC_Send(TELEMETRY_CHANNEL, tx_packet,
                                       TELEMETRY_PACKET_SIZE);
                }
                sequence++;
                sample_count = 0;
            }
        }
    }
}
