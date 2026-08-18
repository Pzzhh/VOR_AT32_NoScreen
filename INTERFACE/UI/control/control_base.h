#ifndef  __control_VOR_H_
#define __control_VOR_H_
#include "stdint.h"
#include "task_control.h"
#ifdef __cplusplus
extern "C"
{
#endif //

	typedef struct
	{
		const char* (*Name)();
		void (*Init)(Task_Parameter_Struct* e);
		void (*pause)(uint8_t enable);
		uint8_t (*getstatus)(uint32_t* remainingCount, uint32_t* parcent);
		void (*stop)();
	}control_cb_struct;
#ifdef __cplusplus
}
#endif // 

#endif // 
