#ifndef  __control_OVAR_H_
#define __control_OVAR_H_
#include "stdint.h"
#include "control.h"
#ifdef __cplusplus
extern "C"
{
#endif //
	uint8_t	 OvarControlFunction(Task_Parameter_Struct* task,
		Task_control_info* e,
		uint8_t isretract);
#ifdef __cplusplus
}
#endif // 

#endif // 
