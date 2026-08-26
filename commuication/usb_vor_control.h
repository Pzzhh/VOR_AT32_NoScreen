#ifndef __USB_VOR_CONTROL_H
#define __USB_VOR_CONTROL_H

#include "stdint.h"
#include "../INTERFACE/UI/control/control.h"
#include "../INTERFACE/UI/control/control_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    USB_VOR_STATE_IDLE = 0,
    USB_VOR_STATE_STARTING,
    USB_VOR_STATE_RUNNING,
    USB_VOR_STATE_PAUSED,
    USB_VOR_STATE_STOPPING,
    USB_VOR_STATE_STOPPED,
    USB_VOR_STATE_READY,
    USB_VOR_STATE_ERROR
} USB_VOR_State;

typedef enum {
    USB_VOR_CMD_NONE = 0,
    USB_VOR_CMD_START,
    USB_VOR_CMD_PAUSE,
    USB_VOR_CMD_RESUME,
    USB_VOR_CMD_STOP,
    USB_VOR_CMD_STATUS,
    USB_VOR_CMD_NEXT,
    USB_VOR_CMD_EXIT
} USB_VOR_Command_Type;

typedef struct {
    USB_VOR_Command_Type type;
    uint32_t id;
    char name[16];
    Task_Parameter_Struct task;
} USB_VOR_Command;

/* Returns 1 when the line belongs to the Ctrl:VOR protocol. */
uint8_t USB_VOR_Command_Process(const char *line);
void USB_VOR_Report_Line_Too_Long(void);

/* Called only by the communication thread. */
void USB_VOR_Communication_Poll(void);

/* The APIs below are consumed only by the task-control thread. */
uint8_t USB_VOR_Task_Peek_Start(USB_VOR_Command *command);
uint8_t USB_VOR_Task_Take_Start(USB_VOR_Command *command);
uint8_t USB_VOR_Task_Command_Pop(USB_VOR_Command *command);
void USB_VOR_Task_Reply(const USB_VOR_Command *command, uint8_t ack,
                        Control_Result result, const char *message);
void USB_VOR_Task_Update(USB_VOR_State state, Control_Result result,
                         const char *event_name, uint32_t progress,
                         uint32_t remaining);
void USB_VOR_Task_Update_Encoder(float angle, uint32_t progress,
                                 uint32_t remaining);
void USB_VOR_Task_Set_Mode(Task_Parameter_Mode_enum mode);
void USB_VOR_Task_Reset(void);

#ifdef __cplusplus
}
#endif

#endif
