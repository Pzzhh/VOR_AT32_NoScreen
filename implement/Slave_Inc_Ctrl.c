#include "at32f435_437.h"
#include "Slave_Inc_Ctrl.h"
#include <math.h>
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"
#include "../HARDWARE/INC/INC_CTRl.h"
#include "../HARDWARE/Motor/inc_motor.h"
#include "../commuication/mpu/jy60.h"
#include "../INTERFACE/UI/other/system_function.h"

#define INC_TOLERANCE 0.2f
/* Avoid classifying an exact 0.2-degree subtraction as outside due to float rounding. */
#define INC_FLOAT_EPSILON 0.00001f
#define INC_STABLE_MS 300U
#define INC_TIMEOUT_MS 60000U
#define INC_SENSOR_MAX_AGE_MS 1000U
#define INC_NO_PROGRESS_MS 10000U
/* Match the old 0.1-degree division; smaller wobble is not useful progress. */
#define INC_PROGRESS_DEGREES 0.1f

static struct {
    volatile INC_Result result;
    volatile uint8_t manual;
    int8_t direction;
    uint8_t reverse;
    uint8_t stable;
    float target;
    uint32_t started;
    uint32_t stable_since;
    float progress_error;
    uint32_t progress_since;
} inc;

static void INC_Disable(void)
{
    INC_IO_Set(0);
    Motor_Ctl_Set(0, 0);
}

static uint8_t INC_Finish(INC_Result result)
{
    INC_Disable();
    inc.result = result;
    inc.manual = 0;
    return 1;
}

uint8_t INC_handler(void)
{
    float angle;
    float error;
    uint32_t now = ControlGetTick();
    if (inc.result != INC_RESULT_MOVING)
        return INC_Finish(inc.result);
    if (overCurrent())
        return INC_Finish(INC_RESULT_OVERCURRENT);
    if (inc.manual) {
        uint8_t io_command;
        if ((uint32_t)(now - inc.started) >= 1000U || inc.direction == 0)
            return INC_Finish(INC_RESULT_OK);
        io_command = inc.direction > 0 ? (inc.reverse ? 2 : 1) : (inc.reverse ? 1 : 2);
        Motor_Ctl_Set(io_command == 1 ? 1000 : -1000, io_command == 1 ? 1000 : -1000);
        INC_IO_Set(io_command);
        return 0;
    }
    if (!JY60_Get_Inc_Fresh(&angle, INC_SENSOR_MAX_AGE_MS) || !isfinite(angle))
        return INC_Finish(INC_RESULT_SENSOR);
    if ((uint32_t)(now - inc.started) >= INC_TIMEOUT_MS)
        return INC_Finish(INC_RESULT_TIMEOUT);
    error = fabsf(inc.target - angle);
    /* Only cumulative improvement resets this watchdog. Quantization/noise
       toggling around one boundary is not progress, nor is moving away. */
    if (inc.progress_error - error >= INC_PROGRESS_DEGREES - INC_FLOAT_EPSILON) {
        inc.progress_error = error;
        inc.progress_since = now;
    }
    if (error <= INC_TOLERANCE + INC_FLOAT_EPSILON) {
        INC_Disable();
        if (!inc.stable) {
            inc.stable = 1;
            inc.stable_since = now;
        }
        if ((uint32_t)(now - inc.stable_since) >= INC_STABLE_MS)
            return INC_Finish(INC_RESULT_OK);
    } else {
        if ((uint32_t)(now - inc.progress_since) >= INC_NO_PROGRESS_MS)
            return INC_Finish(INC_RESULT_STALLED);
        inc.stable = 0;
        if (inc.target > angle) {
            Motor_Ctl_Set(1000, 1000);
            INC_IO_Set(inc.reverse ? 2 : 1);
        } else {
            Motor_Ctl_Set(-1000, -1000);
            INC_IO_Set(inc.reverse ? 1 : 2);
        }
    }
    return 0;
}

uint8_t INC_Machine_Init(float angle, uint32_t Maxcount, uint8_t inc_dir)
{
    float actual;
    (void)Maxcount;
    if (Slave1_Get_Machine_state() || !isfinite(angle) || angle < 0.0f || angle > 50.0f)
        return 0;
    if (!JY60_Get_Inc_Fresh(&actual, INC_SENSOR_MAX_AGE_MS) || !isfinite(actual)) {
        inc.result = INC_RESULT_SENSOR;
        return 0;
    }
    INC_IO_INIT();
    INC_Disable();
    inc.target = angle;
    inc.reverse = inc_dir;
    inc.manual = 0;
    inc.stable = 0;
    inc.started = ControlGetTick();
    inc.progress_error = fabsf(angle - actual);
    inc.progress_since = inc.started;
    /* Already positioned: never energize the mechanism. */
    inc.result = fabsf(angle - actual) <= INC_TOLERANCE + INC_FLOAT_EPSILON ? INC_RESULT_OK : INC_RESULT_MOVING;
    if (inc.result == INC_RESULT_MOVING)
        Slave1_Set_Machine_Cb(INC_handler);
    return 1;
}

uint8_t INC_Machine_Manual_Ctrl(int8_t direction, uint8_t inc_dir)
{
    if (direction == 0)
        return INC_Machine_Stop();
    if (direction != 1 && direction != -1)
        return 0;
    if (Slave1_Get_Machine_state())
        return 0;
    INC_IO_INIT();
    inc.direction = direction;
    inc.reverse = inc_dir;
    inc.manual = 1;
    inc.started = ControlGetTick();
    inc.result = INC_RESULT_MOVING;
    Slave1_Set_Machine_Cb(INC_handler);
    return 1;
}

uint8_t INC_Machine_Stop(void)
{
    inc.result = INC_RESULT_STOPPED;
    inc.manual = 0;
    INC_Disable();
    return 1;
}

uint8_t INC_Machine_Get_Count(float *angleReq, float *CurrentAngle)
{
    if (angleReq != 0)
        *angleReq = inc.target;
    if (CurrentAngle != 0)
        JY60_Get_Inc_Fresh(CurrentAngle, INC_SENSOR_MAX_AGE_MS);
    return inc.result == INC_RESULT_MOVING;
}

INC_Result INC_Machine_Get_Result(void)
{
    return inc.result;
}
