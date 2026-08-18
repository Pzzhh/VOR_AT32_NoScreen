/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-23 16:18:50
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\UI_Component\UI_Dropdown.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UI_Dropdown.h"

void UI_DropDown_Handler(lv_event_t *e)
{
    UI_Dropdown_Struct *obj = (UI_Dropdown_Struct *)e->user_data;
    if (obj == 0)
        return;
    lv_obj_t *dropdown = obj->dropdown;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        obj->Click_Index = lv_dropdown_get_selected(dropdown);
        if (obj->Click_Index_pt != 0)
        {
            *(obj->Click_Index_pt) = lv_dropdown_get_selected(dropdown);
        }
        if (obj->handle != 0)
        {
            obj->handle(obj);
        }
    }
}

void UI_DropDown_Init(UI_Dropdown_Struct *e)
{
    lv_obj_t *parent = e->parent;
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_t *dropdown = lv_dropdown_create(parent);
    const char *Vor_FreqItem = e->Item;
    char *label_str = e->Label_Str;
    uint16_t x = e->x, y = e->y,
             label_x = e->label_ofs_x, label_y = e->label_ofs_y;

    lv_dropdown_set_options(dropdown, Vor_FreqItem);
    lv_label_set_text(label, label_str);

    lv_obj_align(dropdown, LV_ALIGN_LEFT_MID, x, y);
    lv_obj_align_to(label, dropdown, LV_ALIGN_OUT_LEFT_MID, label_x, label_y);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);

    e->label = label;
    e->dropdown = dropdown;
    lv_obj_add_event_cb(dropdown, UI_DropDown_Handler, LV_EVENT_VALUE_CHANGED, (void *)e);
}
