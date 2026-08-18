#ifndef  __task_stroage_H_
#define __task_stroage_H_
#include "stdint.h"
#include "task_info_struct.h"
#ifdef __cplusplus
extern "C"
{
#endif //

	/* typedef struct
	  {
		 Task_get_data_cb get_cb;
	  }Task_Interface_RW;
	  typedef void (*Task_get_data_cb)(void* e);*/

	typedef void (*Task_Add_cb)(void* e);
	Task_Parameter_Struct* Task_Stroage_add(Task_Parameter_Struct info);
	//删除对象
	uint8_t Task_Stroage_del(Task_Parameter_Struct* info);
	uint8_t Task_Stroage_delAll(void);
	uint8_t Task_Stroage_delByID(short ID);
	uint8_t Task_Stroage_MoveByID(uint16_t swap1, uint16_t swap2);

	uint8_t Task_Stroage_Get(Task_Parameter_Struct* info, short INDEX);
	uint8_t Task_Stroage_Set(Task_Parameter_Struct src, short INDEX);
	uint8_t Task_Stroage_GetIndexByInstant(Task_Parameter_Struct* info, int* Index);
	uint16_t Task_Stroage_GetArray(Task_Parameter_Struct* taskarray, uint16_t arraySize);
	int Task_Stroage_GetSize();

	void Task_Stroage_Init();
	//申请插入
	Task_Parameter_Struct* Task_Stroage_Insert(Task_Parameter_Struct insert_info, uint16_t insert_index);
#ifdef __cplusplus
}
#endif // 

#endif // 
