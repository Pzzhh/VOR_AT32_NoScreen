/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-30 15:43:59
 * @FilePath: \USERd:\workfile\项目3 vor\software\VHIT_V2.0\INTERFACE\UI\control\control_VHIT.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-26 16:49:39
 * @FilePath: \USERd:\workfile\项目3 vor\software\VHIT_V2.0\INTERFACE\UI\control\control_VHIT.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef  __control_VHIT_H_
#define __control_VHIT_H_
#include "stdint.h"
#include "control.h"
#include "control_base.h"
#ifdef __cplusplus
extern "C"
{
#endif //
	uint8_t VHITControlFunction(Task_Parameter_Struct *task, Task_control_info *e);
	const Control_Driver *VHIT_Get_Driver(void);
#ifdef __cplusplus
}
#endif // 

#endif // 
