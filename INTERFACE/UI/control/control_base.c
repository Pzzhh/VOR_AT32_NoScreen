#include "control_base.h"
#include "control_Hardware_API.h"
#include "../other/system_function.h"
#include "control.h"
#include <stdio.h>
#include <string.h>

extern void Task_Delay_Msg(char *format, int sec);

static Control_Preview_Status preview_status;

static void Preview_Update(Control_State state, Control_Result result,
                           const char *mode, uint32_t task_index,
                           uint32_t progress, uint32_t remaining,
                           uint8_t can_pause, uint8_t can_resume,
                           uint8_t can_stop, uint8_t needs_input,
                           const char *message)
{
    preview_status.version = CONTROL_PREVIEW_VERSION;
    preview_status.sequence++;
    preview_status.timestamp_ms = ControlGetTick();
    preview_status.state = state;
    preview_status.result = result;
    preview_status.task_index = task_index;
    preview_status.progress = (progress > 100U) ? 100U : progress;
    preview_status.remaining = remaining;
    preview_status.can_pause = can_pause;
    preview_status.can_resume = can_resume;
    preview_status.can_stop = can_stop;
    preview_status.needs_input = needs_input;
    snprintf(preview_status.mode, sizeof(preview_status.mode), "%s",
             mode != 0 ? mode : "UNKNOWN");
    snprintf(preview_status.message, sizeof(preview_status.message), "%s",
             message != 0 ? message : "");
}

void Control_Preview_Init(void)
{
    memset(&preview_status, 0, sizeof(preview_status));
    Preview_Update(CONTROL_STATE_IDLE, CONTROL_RESULT_OK, "NONE", 0, 0, 0,
                   0, 0, 0, 0, "idle");
}

Control_Result Control_Preview_Get_Status(Control_Preview_Status *status)
{
    if (status == 0)
        return CONTROL_RESULT_INVALID_ARGUMENT;
    *status = preview_status;
    return CONTROL_RESULT_OK;
}

void Control_Preview_Set_Unsupported(uint32_t task_index, uint8_t mode)
{
    char mode_name[CONTROL_MODE_NAME_SIZE];
    snprintf(mode_name, sizeof(mode_name), "MODE_%u", (unsigned int)mode);
    Preview_Update(CONTROL_STATE_ERROR, CONTROL_RESULT_UNSUPPORTED_MODE,
                   mode_name, task_index, 0, 0, 0, 0, 0, 0,
                   "unsupported mode");
}

static uint8_t Driver_Is_Valid(const Control_Driver *driver)
{
    return driver != 0 && driver->name != 0 && driver->start != 0 &&
           driver->get_state != 0 && driver->stop != 0;
}

Control_Result Control_Run(Task_Parameter_Struct *task,
                           Task_control_info *control,
                           const Control_Driver *driver)
{
    uint8_t camera_state;
    uint8_t machine_state;
    int32_t last_remaining = -1;
    uint32_t remaining = 0;
    uint32_t percent = 0;
    const char *mode_name;
    uint16_t poll_ms;

    if (task == 0 || control == 0 || !Driver_Is_Valid(driver))
        return CONTROL_RESULT_INVALID_ARGUMENT;

    poll_ms = driver->poll_interval_ms != 0 ? driver->poll_interval_ms : 50U;
    mode_name = driver->name(task);
    control->UI_para.state = taskinit;
    control->State_Bit.reqPause = 0;
    control->State_Bit.isPause = 0;
    Preview_Update(CONTROL_STATE_PREPARING, CONTROL_RESULT_OK, mode_name,
                   control->currentCount, 0, 0, 0, 0, 1, 0, "preparing");

Pause_Resume:
    control->Rk3588_Flag.Lflag = SendTaskArray;
    control->Rk3588_Flag.Rflag = SendTaskArray;
    Task_Delay_Msg("Rec After %d", 5);
    camera_state = HAL_CAM_REC_Set(1);
    Task_Delay_Msg("Run After %d", 10);

    if (!driver->start(task)) {
        Preview_Update(CONTROL_STATE_ERROR, CONTROL_RESULT_START_FAILED,
                       mode_name, control->currentCount, 0, 0,
                       0, 0, 0, 0, "device start failed");
        control->UI_para.state = taskTerminal;
        HAL_CAM_REC_Set(0);
        return CONTROL_RESULT_START_FAILED;
    }

    control->UI_para.state = taskruning;
    Preview_Update(CONTROL_STATE_RUNNING, CONTROL_RESULT_OK, mode_name,
                   control->currentCount, 0, 0,
                   driver->pause != 0, 0, 1, 0, "running");

    while (1) {
        machine_state = driver->get_state(&remaining, &percent);

        if (last_remaining != (int32_t)remaining && machine_state == Imp_running) {
            Ctrl_Msg_Printf("%d:%s Done:%d%%", control->currentCount + 1,
                            mode_name, percent);
            Preview_Update(CONTROL_STATE_RUNNING, CONTROL_RESULT_OK, mode_name,
                           control->currentCount, percent, remaining,
                           driver->pause != 0, 0, 1, 0, "running");
            last_remaining = (int32_t)remaining;
        }

        if (control->State_Bit.Exit) {
            Preview_Update(CONTROL_STATE_STOPPING, CONTROL_RESULT_OK, mode_name,
                           control->currentCount, percent, remaining,
                           0, 0, 0, 0, "stopping");
            Ctrl_Msg_Printf("%d:%s #A52A2A Terminated#",
                            control->currentCount + 1, mode_name);
            control->UI_para.state = taskTerminal;
            control->State_Bit.isPause = 0;
            if (!driver->stop()) {
                Preview_Update(CONTROL_STATE_ERROR, CONTROL_RESULT_STOP_FAILED,
                               mode_name, control->currentCount, percent, remaining,
                               0, 0, 0, 0, "device stop failed");
                HAL_CAM_REC_Set(0);
                return CONTROL_RESULT_STOP_FAILED;
            }
            Preview_Update(CONTROL_STATE_STOPPED, CONTROL_RESULT_CANCELLED,
                           mode_name, control->currentCount, percent, remaining,
                           0, 0, 0, 0, "cancelled");
            break;
        }

        if (control->State_Bit.reqPause) {
            control->State_Bit.reqPause = 0;
            if (machine_state == Imp_running && driver->pause != 0) {
                if (!driver->pause(1)) {
                    Preview_Update(CONTROL_STATE_ERROR, CONTROL_RESULT_PAUSE_FAILED,
                                   mode_name, control->currentCount, percent, remaining,
                                   0, 0, 1, 0, "pause failed");
                    return CONTROL_RESULT_PAUSE_FAILED;
                }
                control->State_Bit.isPause = 1;
                control->UI_para.state = taskPause;
                control->Rk3588_Flag.Lflag = Pause;
                control->Rk3588_Flag.Rflag = Pause;
                HAL_CAM_REC_Set(0);
                Ctrl_Msg_Printf("%d:%s Pause", control->currentCount + 1, mode_name);
                Preview_Update(CONTROL_STATE_PAUSED, CONTROL_RESULT_OK, mode_name,
                               control->currentCount, percent, remaining,
                               0, 1, 1, 0, "paused");
            } else if (machine_state == Imp_paused && driver->pause != 0) {
                control->State_Bit.isPause = 0;
                Task_Delay_Msg("Resume After %d", 5);
                goto Pause_Resume;
            }
        }

        if (driver->on_tick != 0)
            driver->on_tick(control);
        if (machine_state == Imp_finsih)
            break;
        SaftExitDelay(poll_ms, 0);
    }

    if (!control->State_Bit.Exit) {
        Ctrl_Msg_Printf("%d:%s Done:100%%", control->currentCount + 1, mode_name);
        Preview_Update(CONTROL_STATE_COMPLETED, CONTROL_RESULT_OK, mode_name,
                       control->currentCount, 100, 0, 0, 0, 0, 0, "completed");
    }

    SaftExitDelay(1000, 0);
    camera_state = HAL_CAM_REC_Set(0);
    if (!camera_state)
        Ctrl_Msg_Printf("CAM ERROR");
    control->UI_para.state = taskend;
    return CONTROL_RESULT_OK;
}
