#include "usb_vor_control.h"
#include "communication_API.h"
#include "../INTERFACE/UI/control/control_VOR.h"
#include "../INTERFACE/UI/control/control_base.h"
#include "../INTERFACE/UI/control/control_Hardware_API.h"
#include "../INTERFACE/UI/other/system_function.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_VOR_COMMAND_DEPTH 4U
#define USB_VOR_LINE_SIZE 128U

typedef struct {
    volatile USB_VOR_State state;
    volatile uint32_t sequence;
    volatile uint32_t progress;
    volatile uint32_t remaining;
    volatile Control_Result result;
    volatile uint8_t session_active;
} USB_VOR_Status;

static USB_VOR_Command command_queue[USB_VOR_COMMAND_DEPTH];
static volatile uint8_t command_head;
static volatile uint8_t command_tail;
static USB_VOR_Status status = {
    .state = USB_VOR_STATE_IDLE,
    .result = CONTROL_RESULT_OK
};
static volatile Task_Parameter_Mode_enum active_mode = Task_VOR;

extern Task_control_info control_info;

static const char *State_Name(USB_VOR_State state)
{
    switch (state) {
    case USB_VOR_STATE_IDLE: return "IDLE";
    case USB_VOR_STATE_STARTING: return "STARTING";
    case USB_VOR_STATE_RUNNING: return "RUNNING";
    case USB_VOR_STATE_PAUSED: return "PAUSED";
    case USB_VOR_STATE_STOPPING: return "STOPPING";
    case USB_VOR_STATE_STOPPED: return "STOPPED";
    case USB_VOR_STATE_READY: return "READY";
    case USB_VOR_STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

static const char *Task_Mode_Name(Task_Parameter_Mode_enum mode)
{
    switch (mode) {
    case Task_VOR: return "VOR";
    case Task_Continue: return "CONT";
    case Task_OVAR: return "OVAR";
    case Task_VHIT: return "VHIT";
    case Task_TC: return "TC";
    default: return "UNKNOWN";
    }
}

static void Send_Immediate(uint8_t ack, uint32_t id, const char *command,
                           Control_Result code, const char *message)
{
    HAL_USB_Log_Printf("Ctrl:%s ID=%lu CMD=%s CODE=%u STATE=%s MSG=%s\r\n",
                       ack ? "ACK" : "NACK", (unsigned long)id, command,
                       (unsigned int)code, State_Name(status.state), message);
}

static uint8_t Event_Push_Line(const char *line)
{
    return HAL_USB_ACM_Send((uint8_t *)line, (uint16_t)strlen(line));
}

static void Event_Push_Response(uint8_t ack, uint32_t id, const char *command,
                                Control_Result code, const char *message)
{
    char line[USB_VOR_LINE_SIZE];
    snprintf(line, sizeof(line),
             "Ctrl:%s ID=%lu CMD=%s CODE=%u STATE=%s MSG=%s\r\n",
             ack ? "ACK" : "NACK", (unsigned long)id, command,
             (unsigned int)code, State_Name(status.state), message);
    Event_Push_Line(line);
}

static void Event_Push_State(const char *event_name)
{
    char line[USB_VOR_LINE_SIZE];
    status.sequence++;
    snprintf(line, sizeof(line),
             "Ctrl:EVT SEQ=%lu EVENT=%s MODE=%s STATE=%s CODE=%u PCT=%lu REM=%lu\r\n",
             (unsigned long)status.sequence, event_name, Task_Mode_Name(active_mode),
             State_Name(status.state),
             (unsigned int)status.result, (unsigned long)status.progress,
             (unsigned long)status.remaining);
    Event_Push_Line(line);
}

void USB_VOR_Communication_Poll(void)
{
    /* Compatibility no-op: events now enter the TX Pool directly. */
}

void USB_VOR_Task_Update_Encoder(float angle, uint32_t progress,
                                 uint32_t remaining)
{
    char line[USB_VOR_LINE_SIZE];
    status.progress = progress;
    status.remaining = remaining;
    status.sequence++;
    snprintf(line, sizeof(line),
             "Ctrl:EVT SEQ=%lu EVENT=ENCODER MODE=%s STATE=%s CODE=0 PCT=%lu REM=%lu ANGLE=%.3f\r\n",
             (unsigned long)status.sequence, Task_Mode_Name(active_mode),
             State_Name(status.state), (unsigned long)progress,
             (unsigned long)remaining, (double)angle);
    Event_Push_Line(line);
}

static uint8_t Command_Push(const USB_VOR_Command *command)
{
    uint8_t next = (uint8_t)((command_head + 1U) % USB_VOR_COMMAND_DEPTH);
    if (next == command_tail)
        return 0;
    command_queue[command_head] = *command;
    command_head = next;
    return 1;
}

static uint8_t Command_Pop(USB_VOR_Command *command)
{
    if (command_tail == command_head)
        return 0;
    *command = command_queue[command_tail];
    command_tail = (uint8_t)((command_tail + 1U) % USB_VOR_COMMAND_DEPTH);
    return 1;
}

static uint8_t Parse_U32(const char *text, uint32_t *value)
{
    char *end;
    unsigned long parsed;
    if (text == 0 || *text == '\0' || *text == '-')
        return 0;
    parsed = strtoul(text, &end, 10);
    if (*end != '\0')
        return 0;
    *value = (uint32_t)parsed;
    return 1;
}

static uint8_t Parse_Float(const char *text, float *value)
{
    char *end;
    float parsed;
    if (text == 0 || *text == '\0')
        return 0;
    parsed = strtof(text, &end);
    if (*end != '\0')
        return 0;
    *value = parsed;
    return 1;
}

static uint8_t Parse_Common_ID(char *token, uint32_t *id, uint8_t *seen_id)
{
    uint32_t value;
    if (strncmp(token, "ID=", 3) != 0)
        return 0;
    if (*seen_id || !Parse_U32(token + 3, &value))
        return 2;
    *seen_id = 1;
    *id = value;
    return 1;
}

static uint8_t Parse_Start(char *save, USB_VOR_Command *command,
                           Task_Parameter_Mode_enum mode)
{
    uint8_t seen_id = 0;
    uint8_t fields = 0;
    char *token;
    command->task.mode = mode;

    while ((token = strtok_r(0, " ", &save)) != 0) {
        uint8_t id_result = Parse_Common_ID(token, &command->id, &seen_id);
        uint32_t integer_value;
        float float_value;
        if (id_result == 1)
            continue;
        if (id_result == 2)
            return 0;

        if (strlen(token) < 3U || token[1] != '=')
            return 0;

#define USB_SET_U32(bit, target) do {                                      \
            if ((fields & (bit)) || !Parse_U32(token + 2, &integer_value))  \
                return 0;                                                   \
            (target) = integer_value;                                       \
            fields |= (bit);                                                \
        } while (0)
#define USB_SET_FLOAT(bit, target) do {                                    \
            if ((fields & (bit)) || !Parse_Float(token + 2, &float_value))  \
                return 0;                                                    \
            (target) = float_value;                                          \
            fields |= (bit);                                                 \
        } while (0)

        switch (mode) {
        case Task_VOR:
            if (token[0] == 'C') USB_SET_U32(0x01, command->task.VOR.Counter);
            else if (token[0] == 'V') USB_SET_FLOAT(0x02, command->task.VOR.Vel);
            else if (token[0] == 'F') USB_SET_FLOAT(0x04, command->task.VOR.Freq);
            else if (token[0] == 'E') {
                USB_SET_U32(0x08, integer_value);
                command->task.VOR.ExMode = (Task_Parameter_VOR_Exmode)integer_value;
            } else return 0;
            break;
        case Task_Continue:
            if (token[0] == 'S') {
                USB_SET_U32(0x01, integer_value);
                command->task.CONT.Sec = (int)integer_value;
            } else if (token[0] == 'V') USB_SET_FLOAT(0x02, command->task.CONT.Vel);
            else if (token[0] == 'O') USB_SET_FLOAT(0x04, command->task.CONT.okr_Vel);
            else return 0;
            break;
        case Task_OVAR:
            if (token[0] == 'S') {
                USB_SET_U32(0x01, integer_value);
                command->task.OVAR.Sec = (int)integer_value;
            } else if (token[0] == 'V') USB_SET_FLOAT(0x02, command->task.OVAR.Vel);
            else if (token[0] == 'I') USB_SET_FLOAT(0x04, command->task.OVAR.Inc_Degree);
            else return 0;
            break;
        case Task_VHIT:
            if (token[0] == 'C') USB_SET_U32(0x01, command->task.VHIT.Counter);
            else if (token[0] == 'D') {
                USB_SET_U32(0x02, integer_value);
                command->task.VHIT.IsCW = (uint8_t)integer_value;
            } else return 0;
            break;
        case Task_TC:
            if (token[0] == 'C') USB_SET_U32(0x01, command->task.TC.Counter);
            else if (token[0] == 'V') USB_SET_FLOAT(0x02, command->task.TC.Vel);
            else return 0;
            break;
        default:
            return 0;
        }
    }

#undef USB_SET_U32
#undef USB_SET_FLOAT

    switch (mode) {
    case Task_VOR:
        return fields == 0x0F && command->task.VOR.Counter >= 1U &&
               command->task.VOR.Counter <= 1000U &&
               command->task.VOR.Vel >= -360.0f && command->task.VOR.Vel <= 360.0f &&
               command->task.VOR.Freq > 0.0f && command->task.VOR.Freq <= 11.0f &&
               command->task.VOR.ExMode <= Ex_BOTH_R;
    case Task_Continue:
        return fields == 0x07 && command->task.CONT.Sec >= 1 &&
               command->task.CONT.Sec <= 1000 &&
               command->task.CONT.Vel >= -360.0f && command->task.CONT.Vel <= 360.0f &&
               command->task.CONT.okr_Vel >= -360.0f && command->task.CONT.okr_Vel <= 360.0f;
    case Task_OVAR:
        return fields == 0x07 && command->task.OVAR.Sec >= 1 &&
               command->task.OVAR.Sec <= 1000 &&
               command->task.OVAR.Vel >= -360.0f && command->task.OVAR.Vel <= 360.0f &&
               command->task.OVAR.Inc_Degree >= 0.0f && command->task.OVAR.Inc_Degree <= 50.0f;
    case Task_VHIT:
        return fields == 0x03 && command->task.VHIT.Counter >= 1U &&
               command->task.VHIT.Counter <= 1000U && command->task.VHIT.IsCW <= 1U;
    case Task_TC:
        return fields == 0x03 && command->task.TC.Counter >= 1U &&
               command->task.TC.Counter <= 1000U &&
               command->task.TC.Vel >= -360.0f && command->task.TC.Vel <= 360.0f;
    default:
        return 0;
    }
}

static uint8_t Parse_No_Parameters(char *save, USB_VOR_Command *command)
{
    uint8_t seen_id = 0;
    char *token;
    while ((token = strtok_r(0, " ", &save)) != 0) {
        uint8_t result = Parse_Common_ID(token, &command->id, &seen_id);
        if (result != 1)
            return 0;
    }
    return 1;
}

uint8_t USB_VOR_Command_Process(const char *line)
{
    char buffer[USB_VOR_LINE_SIZE];
    char *save = 0;
    char *verb;
    char mode_text[8];
    char action[16];
    Task_Parameter_Mode_enum requested_mode;
    USB_VOR_Command command;
    uint8_t reserved_owner = 0;

    if (line == 0 || strncmp(line, "Ctrl:", 5) != 0)
        return 0;

    memset(&command, 0, sizeof(command));
    snprintf(buffer, sizeof(buffer), "%s", line);
    verb = strtok_r(buffer, " ", &save);
    if (sscanf(verb, "Ctrl:%7[^.].%15s", mode_text, action) != 2) {
        Send_Immediate(0, 0, "Unknown", CONTROL_RESULT_UNSUPPORTED_MODE,
                       "UNKNOWN_COMMAND");
        return 1;
    }
    if (strcmp(mode_text, "VOR") == 0) requested_mode = Task_VOR;
    else if (strcmp(mode_text, "CONT") == 0) requested_mode = Task_Continue;
    else if (strcmp(mode_text, "OVAR") == 0) requested_mode = Task_OVAR;
    else if (strcmp(mode_text, "VHIT") == 0) requested_mode = Task_VHIT;
    else if (strcmp(mode_text, "TC") == 0) requested_mode = Task_TC;
    else {
        Send_Immediate(0, 0, "Unknown", CONTROL_RESULT_UNSUPPORTED_MODE,
                       "UNSUPPORTED_MODE");
        return 1;
    }

    snprintf(command.name, sizeof(command.name), "%.7s.%.7s", mode_text, action);
    if (strcmp(action, "Strat") == 0 || strcmp(action, "Start") == 0) {
        command.type = USB_VOR_CMD_START;
        if (!Parse_Start(save, &command, requested_mode)) {
            Send_Immediate(0, command.id, command.name,
                           CONTROL_RESULT_INVALID_ARGUMENT, "INVALID_PARAM");
            return 1;
        }
        if (control_info.controlOwner != CONTROL_OWNER_NONE ||
            control_info.State_Bit.IsRunning) {
            Send_Immediate(0, command.id, command.name,
                           CONTROL_RESULT_BUSY, "BUSY");
            return 1;
        }
        if (control_info.controlOwner == CONTROL_OWNER_NONE) {
            control_info.controlOwner = CONTROL_OWNER_USB;
            active_mode = requested_mode;
            reserved_owner = 1;
        }
    } else if (strcmp(action, "Pause") == 0) {
        command.type = USB_VOR_CMD_PAUSE;
    } else if (strcmp(action, "Resume") == 0) {
        command.type = USB_VOR_CMD_RESUME;
    } else if (strcmp(action, "Stop") == 0) {
        command.type = USB_VOR_CMD_STOP;
    } else if (strcmp(action, "Status") == 0) {
        command.type = USB_VOR_CMD_STATUS;
    } else if (strcmp(action, "Next") == 0) {
        command.type = USB_VOR_CMD_NEXT;
    } else if (strcmp(action, "Exit") == 0) {
        command.type = USB_VOR_CMD_EXIT;
    } else {
        Send_Immediate(0, 0, command.name, CONTROL_RESULT_UNSUPPORTED_MODE,
                       "UNKNOWN_COMMAND");
        return 1;
    }

    if (command.type != USB_VOR_CMD_START &&
        !Parse_No_Parameters(save, &command)) {
        Send_Immediate(0, command.id, command.name,
                       CONTROL_RESULT_INVALID_ARGUMENT, "INVALID_PARAM");
        return 1;
    }

    if (command.type != USB_VOR_CMD_START &&
        control_info.controlOwner != CONTROL_OWNER_USB) {
        Send_Immediate(0, command.id, command.name,
                       CONTROL_RESULT_INVALID_STATE, "NO_PC_SESSION");
        return 1;
    }
    if (command.type != USB_VOR_CMD_START && requested_mode != active_mode) {
        Send_Immediate(0, command.id, command.name,
                       CONTROL_RESULT_INVALID_STATE, "WRONG_MODE");
        return 1;
    }
    if (command.type == USB_VOR_CMD_NEXT && requested_mode != Task_VHIT) {
        Send_Immediate(0, command.id, command.name,
                       CONTROL_RESULT_UNSUPPORTED_MODE, "UNSUPPORTED_COMMAND");
        return 1;
    }

    if (!Command_Push(&command)) {
        if (reserved_owner)
            control_info.controlOwner = CONTROL_OWNER_NONE;
        Send_Immediate(0, command.id, command.name,
                       CONTROL_RESULT_QUEUE_FULL, "QUEUE_FULL");
    }
    return 1;
}

void USB_VOR_Report_Line_Too_Long(void)
{
    Send_Immediate(0, 0, "VOR.Unknown", CONTROL_RESULT_LINE_TOO_LONG,
                   "LINE_TOO_LONG");
}

uint8_t USB_VOR_Task_Peek_Start(USB_VOR_Command *command)
{
    if (command == 0 || command_tail == command_head ||
        command_queue[command_tail].type != USB_VOR_CMD_START)
        return 0;
    *command = command_queue[command_tail];
    return 1;
}

uint8_t USB_VOR_Task_Take_Start(USB_VOR_Command *command)
{
    if (!USB_VOR_Task_Peek_Start(command))
        return 0;
    return Command_Pop(command);
}

uint8_t USB_VOR_Task_Command_Pop(USB_VOR_Command *command)
{
    if (command == 0)
        return 0;
    return Command_Pop(command);
}

void USB_VOR_Task_Reply(const USB_VOR_Command *command, uint8_t ack,
                        Control_Result result, const char *message)
{
    if (command == 0)
        return;
    Event_Push_Response(ack, command->id, command->name, result,
                        message != 0 ? message : "");
}

void USB_VOR_Task_Update(USB_VOR_State state, Control_Result result,
                         const char *event_name, uint32_t progress,
                         uint32_t remaining)
{
    status.state = state;
    status.result = result;
    status.progress = progress > 100U ? 100U : progress;
    status.remaining = remaining;
    status.session_active = 1;
    if (event_name != 0)
        Event_Push_State(event_name);
}

void USB_VOR_Task_Set_Mode(Task_Parameter_Mode_enum mode)
{
    active_mode = mode;
}

void USB_VOR_Task_Reset(void)
{
    /* Commands queued after Stop/Exit belong to the completed one-shot task. */
    command_tail = command_head;
    status.state = USB_VOR_STATE_IDLE;
    status.result = CONTROL_RESULT_OK;
    status.progress = 0;
    status.remaining = 0;
    status.session_active = 0;
}
