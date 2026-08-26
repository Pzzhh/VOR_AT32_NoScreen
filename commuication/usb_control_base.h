#ifndef __USB_CONTROL_BASE_H_
#define __USB_CONTROL_BASE_H_

#include "../INTERFACE/UI/control/control_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB task execution template.
 *
 * This intentionally mirrors Control_Run so the UI-only sections can be
 * reviewed and removed one by one without changing the common driver model.
 */
Control_Result USB_Control_Run(Task_Parameter_Struct *task,
                               Task_control_info *control,
                               const Control_Driver *driver);

#ifdef __cplusplus
}
#endif

#endif
