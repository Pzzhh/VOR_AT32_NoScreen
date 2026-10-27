#include "control_INC.h"
#include "control_Hardware_API.h"
#ifdef ATM32F43_xx
#include "../implement/Slave_Inc_Ctrl.h"
#include "../commuication/mpu/jy60.h"
#endif

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

static Control_Result INC_Driver_Result(void)
{
#ifdef ATM32F43_xx
    switch (INC_Machine_Get_Result()) {
    case INC_RESULT_SENSOR: return CONTROL_RESULT_SENSOR_STALE;
    case INC_RESULT_OVERCURRENT: return CONTROL_RESULT_OVERCURRENT;
    case INC_RESULT_TIMEOUT: return CONTROL_RESULT_TIMEOUT;
    case INC_RESULT_STALLED: return CONTROL_RESULT_NO_PROGRESS;
    case INC_RESULT_STOPPED: return CONTROL_RESULT_CANCELLED;
    default: break;
    }
#endif
    return CONTROL_RESULT_OK;
}

static uint8_t INC_Driver_Angle(float *angle)
{
#ifdef ATM32F43_xx
    return JY60_Get_Inc_Fresh(angle, 1000U);
#else
    *angle = current_angle;
    return 1;
#endif
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
    .poll_interval_ms = 50,
    .get_result = INC_Driver_Result,
    .get_angle = INC_Driver_Angle
};

const Control_Driver *INC_Get_Driver(void)
{
    return &INC_Driver;
}
