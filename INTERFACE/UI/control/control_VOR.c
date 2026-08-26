#include "control_VOR.h"
#include "control_Hardware_API.h"
#include "control_base.h"
#include "../other/system_function.h"
#include "control.h"
#include "stdint.h"

#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "../implement/Slave_Vor_Ctrl.h"
#endif

static struct {
    uint32_t time;
    uint32_t RemainTime;
    uint8_t flag_pause;
    uint32_t SetCount;
    float Vel;
    float Freq;
} vor_info;

uint8_t HAL_Slave_VOR_Init(Task_Parameter_Struct *task)
{
#ifndef ATM32F43_xx
    vor_info.time = ControlGetTick();
    vor_info.Freq = task->VOR.Freq;
    vor_info.SetCount = task->VOR.Counter;
    vor_info.flag_pause = 0;
#else
    if (task->mode == Task_VOR)
        VOR_Machine_Init(task->VOR.Freq, task->VOR.Vel,
                         task->VOR.Counter, task->VOR.ExMode);
#endif
    return 1;
}

uint8_t HAL_Slave_VOR_Stop(void)
{
#ifndef ATM32F43_xx
    uint32_t current_count =
        vor_info.Freq * (ControlGetTick() - vor_info.time) / 1000.0f;
    vor_info.SetCount = current_count + 1;
    vor_info.flag_pause = 0;
#else
    VOR_Machine_Stop();
#endif
    return 1;
}

uint8_t HAL_Slave_VOR_Pause(uint8_t enable)
{
#ifndef ATM32F43_xx
    if (enable) {
        vor_info.flag_pause = 1;
        vor_info.RemainTime = ControlGetTick() - vor_info.time;
    } else {
        vor_info.time = ControlGetTick() - vor_info.RemainTime;
        vor_info.flag_pause = 0;
    }
#else
    VOR_Machine_Pause();
#endif
    return 1;
}

uint8_t HAL_Slave_VOR_Get_State(uint32_t *remaining_count, uint32_t *percent)
{
#ifndef ATM32F43_xx
    uint32_t current_count;
    if (vor_info.flag_pause)
        return Imp_paused;
    current_count =
        vor_info.Freq * (ControlGetTick() - vor_info.time) / 1000.0f;
    if (percent != 0)
        *percent = current_count * 100 / vor_info.SetCount;
    if (current_count < vor_info.SetCount) {
        *remaining_count = vor_info.SetCount - current_count;
        return Imp_running;
    }
    *remaining_count = 0;
    return Imp_finsih;
#else
    uint32_t counter_req = 0;
    uint32_t current_count = 0;
    uint8_t result = VOR_Machine_Get_Count(&counter_req, &current_count);
    if (percent != 0 && counter_req != 0)
        *percent = current_count * 100 / counter_req;
    *remaining_count = counter_req - current_count;
    return result;
#endif
}

const char *ModeStr(Task_Parameter_Struct *task)
{
    static const char names[][10] = {"VOR", "OKR", "VOR+OKR", "VOR+OKR*"};
    return names[task->VOR.ExMode];
}

static const char *VOR_Driver_Name(const Task_Parameter_Struct *task)
{
    return ModeStr((Task_Parameter_Struct *)task);
}

static const Control_Driver VOR_Driver = {
    .name = VOR_Driver_Name,
    .start = HAL_Slave_VOR_Init,
    .pause = HAL_Slave_VOR_Pause,
    .get_state = HAL_Slave_VOR_Get_State,
    .stop = HAL_Slave_VOR_Stop,
    .next = 0,
    .on_tick = 0,
    .poll_interval_ms = 50
};

const Control_Driver *VOR_Get_Driver(void)
{
    return &VOR_Driver;
}

uint16_t VorControlFunction(Task_Parameter_Struct *task, Task_control_info *control)
{
    return (uint16_t)Control_Run(task, control, &VOR_Driver);
}
