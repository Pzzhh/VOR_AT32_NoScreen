#include "usb_control_base.h"
#include "usb_vor_control.h"
#include "../INTERFACE/UI/control/control_Hardware_API.h"
#include "../INTERFACE/UI/other/system_function.h"

static uint8_t USB_Driver_Is_Valid(const Control_Driver *driver)
{
    return driver != 0 && driver->name != 0 && driver->start != 0 &&
           driver->get_state != 0 && driver->stop != 0;
}

static void USB_Reply_Invalid_State(const USB_VOR_Command *command)
{
    USB_VOR_Task_Reply(command, 0, CONTROL_RESULT_INVALID_STATE,
                       "INVALID_STATE");
}

static uint32_t USB_Initial_Remaining(const Task_Parameter_Struct *task)
{
    switch (task->mode) {
    case Task_VOR: return task->VOR.Counter;
    case Task_Continue: return (uint32_t)task->CONT.Sec;
    case Task_OVAR: return (uint32_t)task->OVAR.Sec;
    case Task_VHIT: return task->VHIT.Counter;
    case Task_TC: return task->TC.Counter;
    case Task_Incline: return 1U;
    default: return 0;
    }
}

static uint8_t USB_Is_Rotation_Mode(Task_Parameter_Mode_enum mode)
{
    return mode == Task_VOR || mode == Task_Continue ||
           mode == Task_OVAR || mode == Task_VHIT || mode == Task_TC;
}

static uint8_t USB_Wait_Machine(const Control_Driver *driver, uint8_t target,
                                uint32_t *remaining, uint32_t *percent)
{
    uint32_t started = ControlGetTick();
    do {
        if (driver->get_state(remaining, percent) == target)
            return 1;
        if ((uint32_t)(ControlGetTick() - started) >= 3000U)
            return 0;
        SaftExitDelay(10, 0);
    } while (1);
}

Control_Result USB_Control_Run(Task_Parameter_Struct *task,
                               Task_control_info *control,
                               const Control_Driver *driver)
{
    USB_VOR_Command start_command;
    USB_VOR_Command command;
    uint8_t machine_state = Imp_running;
    uint8_t paused = 0;
    uint32_t remaining;
    uint32_t percent = 0;
    int32_t last_remaining = -1;
    uint16_t poll_ms;

    if (task == 0 || control == 0 || !USB_Driver_Is_Valid(driver))
        return CONTROL_RESULT_INVALID_ARGUMENT;
    if (!USB_VOR_Task_Take_Start(&start_command))
        return CONTROL_RESULT_INVALID_STATE;
    USB_VOR_Task_Set_Request(start_command.id);

    poll_ms = driver->poll_interval_ms != 0 ? driver->poll_interval_ms : 50U;
    remaining = USB_Initial_Remaining(task);
    control->UI_para.state = taskinit;
    control->State_Bit.reqPause = 0;
    control->State_Bit.isPause = 0;

    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "COMMAND_RECEIVED", 0, remaining);
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "COMMAND_PARSED", 0, remaining);
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "PARAM_VALIDATED", 0, remaining);
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "STARTING", 0, remaining);

    /* UI TEMPLATE: RK3588 task/recording messages are bypassed for USB. */
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "RK3588_BYPASSED", 0, remaining);
    /* UI TEMPLATE: camera recording is bypassed for USB. */
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "CAMERA_BYPASSED", 0, remaining);
    /* UI TEMPLATE: 5/10 second recording delays are bypassed for USB. */
    USB_VOR_Task_Update(USB_VOR_STATE_STARTING, CONTROL_RESULT_OK,
                        "RECORD_DELAY_BYPASSED", 0, remaining);

    if (!driver->start(task)) {
        Control_Result failure = driver->get_result != 0 ? driver->get_result() : CONTROL_RESULT_START_FAILED;
        if (failure == CONTROL_RESULT_OK)
            failure = CONTROL_RESULT_START_FAILED;
        control->UI_para.state = taskTerminal;
        USB_VOR_Task_Reply(&start_command, 0, failure,
                           "START_FAILED");
        USB_VOR_Task_Update(USB_VOR_STATE_ERROR, failure,
                            "ERROR", 0, remaining);
        return failure;
    }

    control->State_Bit.IsRunning = 1;
    control->UI_para.state = taskruning;
    USB_VOR_Task_Reply(&start_command, 1, CONTROL_RESULT_OK, "STARTED");
    USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                        "MOTOR_START", 0, remaining);
    if (USB_Is_Rotation_Mode(task->mode)) {
        HAL_CAM_SET_sign_led();
        SaftExitDelay(50, 0);
        HAL_CAM_SET_sign_led();
    }
    USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                        "RUNNING", 0, remaining);

    while (1) {
        while (USB_VOR_Task_Command_Pop(&command)) {
            switch (command.type) {
            case USB_VOR_CMD_PAUSE:
                if (paused || machine_state != Imp_running || driver->pause == 0) {
                    USB_Reply_Invalid_State(&command);
                    break;
                }
                USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                                    "PAUSING", percent, remaining);
                if (!driver->pause(1) || (task->mode == Task_OVAR &&
                    !USB_Wait_Machine(driver, Imp_paused, &remaining, &percent))) {
                    USB_VOR_Task_Reply(&command, 0,
                                       CONTROL_RESULT_PAUSE_FAILED,
                                       "PAUSE_FAILED");
                    USB_VOR_Task_Update(USB_VOR_STATE_ERROR,
                                        CONTROL_RESULT_PAUSE_FAILED, "ERROR",
                                        percent, remaining);
                    return CONTROL_RESULT_PAUSE_FAILED;
                }
                paused = 1;
                control->State_Bit.isPause = 1;
                control->UI_para.state = taskPause;
                USB_VOR_Task_Reply(&command, 1, CONTROL_RESULT_OK, "PAUSED");
                USB_VOR_Task_Update(USB_VOR_STATE_PAUSED, CONTROL_RESULT_OK,
                                    "PAUSED", percent, remaining);
                break;

            case USB_VOR_CMD_RESUME:
                if (!paused || driver->pause == 0) {
                    USB_Reply_Invalid_State(&command);
                    break;
                }
                USB_VOR_Task_Update(USB_VOR_STATE_PAUSED, CONTROL_RESULT_OK,
                                    "RESUMING", percent, remaining);
                if (!driver->pause(0)) {
                    USB_VOR_Task_Reply(&command, 0,
                                       CONTROL_RESULT_RESUME_FAILED,
                                       "RESUME_FAILED");
                    USB_VOR_Task_Update(USB_VOR_STATE_ERROR,
                                        CONTROL_RESULT_RESUME_FAILED, "ERROR",
                                        percent, remaining);
                    return CONTROL_RESULT_RESUME_FAILED;
                }
                paused = 0;
                control->State_Bit.isPause = 0;
                control->UI_para.state = taskruning;
                USB_VOR_Task_Reply(&command, 1, CONTROL_RESULT_OK, "RUNNING");
                USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                                    "RUNNING", percent, remaining);
                break;

            case USB_VOR_CMD_STOP:
            case USB_VOR_CMD_EXIT:
                USB_VOR_Task_Update(USB_VOR_STATE_STOPPING,
                                    CONTROL_RESULT_CANCELLED, "STOPPING",
                                    percent, remaining);
                if (!driver->stop() || ((task->mode == Task_OVAR || task->mode == Task_Incline) &&
                    !USB_Wait_Machine(driver, Imp_finsih, &remaining, &percent))) {
                    USB_VOR_Task_Reply(&command, 0,
                                       CONTROL_RESULT_STOP_FAILED,
                                       "STOP_FAILED");
                    USB_VOR_Task_Update(USB_VOR_STATE_ERROR,
                                        CONTROL_RESULT_STOP_FAILED, "ERROR",
                                        percent, remaining);
                    return CONTROL_RESULT_STOP_FAILED;
                }
                control->State_Bit.isPause = 0;
                control->UI_para.state = taskend;
                USB_VOR_Task_Reply(&command, 1, CONTROL_RESULT_OK,
                                   command.type == USB_VOR_CMD_EXIT ?
                                   "EXITED" : "STOPPED");
                USB_VOR_Task_Update(USB_VOR_STATE_STOPPED,
                                    CONTROL_RESULT_CANCELLED, "STOPPED",
                                    percent, remaining);
                return CONTROL_RESULT_CANCELLED;

            case USB_VOR_CMD_STATUS:
                USB_VOR_Task_Reply(&command, 1, CONTROL_RESULT_OK, "STATUS");
                USB_VOR_Task_Update(paused ? USB_VOR_STATE_PAUSED :
                                    USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                                    "STATUS", percent, remaining);
                break;

            case USB_VOR_CMD_NEXT:
                if (driver->next == 0 || paused) {
                    USB_Reply_Invalid_State(&command);
                    break;
                }
                if (!driver->next()) {
                    USB_VOR_Task_Reply(&command, 0,
                                       CONTROL_RESULT_INTERNAL_ERROR,
                                       "NEXT_FAILED");
                    break;
                }
                USB_VOR_Task_Reply(&command, 1, CONTROL_RESULT_OK, "NEXT");
                USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                                    "NEXT", percent, remaining);
                break;

            case USB_VOR_CMD_START:
            case USB_VOR_CMD_NONE:
            default:
                USB_Reply_Invalid_State(&command);
                break;
            }
        }

        machine_state = driver->get_state(&remaining, &percent);
        if (driver->get_result != 0) {
            Control_Result result = driver->get_result();
            if (result != CONTROL_RESULT_OK) {
                driver->stop();
                control->UI_para.state = taskTerminal;
                USB_VOR_Task_Update(USB_VOR_STATE_ERROR, result, "ERROR", percent, remaining);
                return result;
            }
        }
        if (percent > 100U)
            percent = 100U;
        if (!paused && machine_state == Imp_running &&
            last_remaining != (int32_t)remaining) {
            last_remaining = (int32_t)remaining;
            USB_VOR_Task_Update(USB_VOR_STATE_RUNNING, CONTROL_RESULT_OK,
                                "PROGRESS", percent, remaining);
        }
        if (machine_state == Imp_finsih) {
            control->UI_para.state = taskend;
            USB_VOR_Task_Update(USB_VOR_STATE_READY, CONTROL_RESULT_OK,
                                "COMPLETED", 100, 0);
            return CONTROL_RESULT_OK;
        }

        /* UI TEMPLATE: UI messages, Wi-Fi checks and beeps are omitted here. */
        if (driver->on_tick != 0)
            driver->on_tick(control);
        SaftExitDelay(poll_ms, 0);
    }
}
