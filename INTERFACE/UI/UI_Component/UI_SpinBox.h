#ifndef  __UI_SpinBox_H_
#define __UI_SpinBox_H_
#include "../UI_Include.h"
#ifdef __cplusplus
extern "C"
{

#endif // 
    typedef struct
    {
        lv_obj_t* parent;
        lv_obj_t* label;
        lv_obj_t* spinbox;
        uint16_t x;
        uint16_t y;
        int32_t* SpinBox_Value;
        char* Label_Str;
        uint16_t MaxValue;
		uint16_t MinValue;
    } UI_SpinBox_Struct;
    void SpinBox_Init(UI_SpinBox_Struct* e);



#ifdef __cplusplus
}
#endif // 

#endif // ! UI_Paremeter_Set_H_
