#include "control_INC.h"
#include "control_Hardware_API.h"

static float current_angle;

static const char *INC_Driver_Name(const Task_Parameter_Struct *task)
{
    (void)task;
    return "INC";
}

static uint8_t INC_Driver_Start(Task_Parameter_Struct *task)
{
    current_angle = 0.0f;
    return HAL_Incline_Init(task->INC.Angle, 0);
}

static uint8_t INC_Driver_Get_State(uint32_t *remaining, uint32_t *percent)
{
    uint8_t running = HAL_Incline_Get_State(&current_angle);
    if (remaining != 0)
        *remaining = running ? 1U : 0U;
    if (percent != 0)
        *percent = running ? 0U : 100U;
    return running ? Imp_running : Imp_finsih;
}

static uint8_t INC_Driver_Stop(void)
{
    return HAL_Incline_Stop();
}

static const Control_Driver INC_Driver = {
    .name = INC_Driver_Name,
    .start = INC_Driver_Start,
    .pause = 0,
    .get_state = INC_Driver_Get_State,
    .stop = INC_Driver_Stop,
    .next = 0,
    .on_tick = 0,
    .poll_interval_ms = 50
};

const Control_Driver *INC_Get_Driver(void)
{
    return &INC_Driver;
}
