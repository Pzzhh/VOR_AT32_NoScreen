#ifndef __CONTROL_BASE_H_
#define __CONTROL_BASE_H_

#include "stdint.h"
#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONTROL_PREVIEW_VERSION 1U
#define CONTROL_MODE_NAME_SIZE 16U
#define CONTROL_MESSAGE_SIZE 64U

typedef enum {
    CONTROL_STATE_IDLE = 0,
    CONTROL_STATE_PREPARING,
    CONTROL_STATE_RUNNING,
    CONTROL_STATE_PAUSED,
    CONTROL_STATE_WAITING_INPUT,
    CONTROL_STATE_STOPPING,
    CONTROL_STATE_STOPPED,
    CONTROL_STATE_COMPLETED,
    CONTROL_STATE_ERROR
} Control_State;

typedef enum {
    CONTROL_RESULT_OK = 0,
    CONTROL_RESULT_INVALID_ARGUMENT = 1001,
    CONTROL_RESULT_UNSUPPORTED_MODE = 1002,
    CONTROL_RESULT_INVALID_STATE = 1003,
    CONTROL_RESULT_BUSY = 1004,
    CONTROL_RESULT_QUEUE_FULL = 1005,
    CONTROL_RESULT_LINE_TOO_LONG = 1006,
    CONTROL_RESULT_START_FAILED = 2001,
    CONTROL_RESULT_QUERY_FAILED = 2002,
    CONTROL_RESULT_PAUSE_FAILED = 2003,
    CONTROL_RESULT_RESUME_FAILED = 2004,
    CONTROL_RESULT_STOP_FAILED = 2005,
    CONTROL_RESULT_TIMEOUT = 2006,
    CONTROL_RESULT_CANCELLED = 3001,
    CONTROL_RESULT_INTERNAL_ERROR = 9001
} Control_Result;

typedef struct {
    uint32_t version;
    uint32_t sequence;
    uint32_t timestamp_ms;
    Control_State state;
    Control_Result result;
    char mode[CONTROL_MODE_NAME_SIZE];
    uint32_t task_index;
    uint32_t progress;
    uint32_t remaining;
    uint8_t can_pause;
    uint8_t can_resume;
    uint8_t can_stop;
    uint8_t needs_input;
    char message[CONTROL_MESSAGE_SIZE];
} Control_Preview_Status;

typedef struct {
    const char *(*name)(const Task_Parameter_Struct *task);
    uint8_t (*start)(Task_Parameter_Struct *task);
    uint8_t (*pause)(uint8_t enable);
    uint8_t (*get_state)(uint32_t *remaining, uint32_t *percent);
    uint8_t (*stop)(void);
    uint8_t (*next)(void);
    void (*on_tick)(Task_control_info *control);
    uint16_t poll_interval_ms;
} Control_Driver;

void Control_Preview_Init(void);
Control_Result Control_Preview_Get_Status(Control_Preview_Status *status);
void Control_Preview_Set_Unsupported(uint32_t task_index, uint8_t mode);
Control_Result Control_Run(Task_Parameter_Struct *task,
                           Task_control_info *control,
                           const Control_Driver *driver);

#ifdef __cplusplus
}
#endif

#endif
