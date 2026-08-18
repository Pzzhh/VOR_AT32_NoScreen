#ifndef  __UI_Parameter_Set_H_
#define __UI_Parameter_Set_H_
#include "../UI_Include.h"
#include "../Task/task_info_struct.h"
#ifdef __cplusplus
extern "C"
{

#endif // 7
    typedef void (*myhander)(void *e);
    typedef struct
    {
        uint8_t(*Get)(Task_Parameter_Struct* e);
        uint8_t(*Set)(Task_Parameter_Struct* e);
        //flag 1->hidden
        uint8_t(*hidden)(uint8_t flag);

    }UI_Parameter_Handler;
    void Mode_init(lv_obj_t* parent, uint8_t isEnOkR);
    uint8_t UI_Parameter_Read(Task_Parameter_Struct* e);

#ifdef __cplusplus

}
#endif // 

#endif // ! UI_Paremeter_Set_H_
