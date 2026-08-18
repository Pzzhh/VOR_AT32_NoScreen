#ifndef __UI_Dropdown_H_
#define __UI_Dropdown_H_
#include "../UI_Include.h"
#ifdef __cplusplus
extern "C"
{

#endif //
    typedef struct
    {
        lv_obj_t *parent;
        lv_obj_t *label;
        lv_obj_t *dropdown;
        uint16_t x;
        uint16_t y;
        uint16_t label_ofs_x;
        uint16_t label_ofs_y;
        uint16_t *Click_Index_pt;
        uint16_t Click_Index;
        const char *Item;
        char *Label_Str;
        void (*handle)(void *e);
    } UI_Dropdown_Struct;
    void UI_DropDown_Init(UI_Dropdown_Struct *e);

#ifdef __cplusplus
}
#endif //

#endif // ! UI_Paremeter_Set_H_
