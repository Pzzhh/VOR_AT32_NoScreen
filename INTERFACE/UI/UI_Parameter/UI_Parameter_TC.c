#include "UI_Parameter_TC.h"
#include "UI_Parameter_Set.h"
#include "../UI_Include.h"
#include "../UI_Component/UI_SpinBox.h"
#include "../UI_Component/UI_Dropdown.h"

const char *TC_VelItem = "5/s\n"
                         "10/s\n"
                         "20/s\n"
                         "30/s\n"
                         "40/s\n"
                         "50/s\n"
                         "60/s\n"
                         "80/s";

typedef struct
{
    struct
    {
        lv_obj_t *label1;
        lv_obj_t *dropdown1;
    } Freq;
    struct
    {
        lv_obj_t *label1;
        lv_obj_t *dropdown1;
        lv_obj_t *btn1_CW_Set;
        lv_obj_t *btn1_CW_label2;
        uint8_t CW_flag;
    } Vel;
    struct
    {
        lv_obj_t *label1;
        lv_obj_t *spinbox1;
        int32_t value;
    } counter;
    lv_obj_t *Context_Box;
} UI_parameter_TC;

static void rotate_dir_buttum_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t *flag = e->user_data;
    lv_obj_t *label = lv_obj_get_child(e->current_target, 0);
    if (code == LV_EVENT_CLICKED)
    {
        (*flag) = !(*flag);
    }
    if (1)
    {
        if (*flag)
            lv_label_set_text(label, "CW");
        else
            lv_label_set_text(label, "CCW");
    }
}

void handler(UI_Dropdown_Struct *e)
{
    LV_LOG_USER("ss %d", e->Click_Index);
}

static UI_Dropdown_Struct dropdown_Vel2, dropdown_vel2;
static UI_parameter_TC Obj;

static uint8_t UI_Get(Task_Parameter_Struct *e);
static uint8_t UI_Hidden(uint8_t flag);

UI_Parameter_Handler TC_init(lv_obj_t *parent, int16_t x, int16_t y)
{
    // create base obj
    UI_Parameter_Handler ui_Parameter_Handler = {.hidden = UI_Hidden, .Get = UI_Get};

    lv_obj_t *context_box = lv_obj_create(parent);
    Obj.Context_Box = context_box;
    lv_obj_clear_flag(context_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(context_box, 315, 260);
    lv_obj_set_style_bg_opa(context_box, 0, LV_OPA_TRANSP);
    lv_obj_set_style_border_opa(context_box, 0, LV_OPA_TRANSP);
    lv_obj_align(context_box, LV_ALIGN_DEFAULT, x, y);

    UI_Dropdown_Struct dropdown_vel = {
        .dropdown = Obj.Vel.dropdown1,
        .label = Obj.Vel.label1,
        .label_ofs_x = -25,
        .label_ofs_y = 0,
        .x = 50,
        .y = -60,
        .Item = TC_VelItem,
        .parent = context_box,
        .handle = (myhander)handler,
        .Label_Str = "Vel"};
    dropdown_vel2 = dropdown_vel;
    UI_DropDown_Init(&dropdown_vel2);
    Obj.Vel.dropdown1 = dropdown_vel2.dropdown;

    Obj.Vel.btn1_CW_Set = lv_btn_create(context_box);
    lv_obj_set_size(Obj.Vel.btn1_CW_Set, 40, 40);
    // label init
    Obj.Vel.btn1_CW_label2 = lv_label_create(Obj.Vel.btn1_CW_Set);
    lv_label_set_text(Obj.Vel.btn1_CW_label2, "CCW");
    lv_obj_align(Obj.Vel.btn1_CW_label2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align_to(Obj.Vel.btn1_CW_Set, Obj.Vel.dropdown1, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    // btn is clicked change label text
    lv_obj_add_event_cb(Obj.Vel.btn1_CW_Set, rotate_dir_buttum_handler, LV_EVENT_CLICKED, (void *)&Obj.Vel.CW_flag);
    // UI_SpinBox_Struct e = {
    //      .parent = Obj.Context_Box,
    //     .SpinBox_Value = &Obj.counter.value,
    //     .x = 50,
    //     .y = -60,
    //     .Label_Str = "Count" };
    // SpinBox_Init(&e);

 /*   Obj.counter.label1 = e.label;
    Obj.counter.spinbox1 = e.spinbox;*/

    return ui_Parameter_Handler;
}

static uint8_t UI_Hidden(uint8_t flag)
{
    static uint8_t Hidden_flag = 0;
    lv_obj_t *parent = (Obj.Context_Box);
    if (lv_obj_is_valid(parent))
    {
        if (flag)
        {
            if (Hidden_flag == 0)
                lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
            Hidden_flag = 1;
        }
        else
        {
            if (Hidden_flag != 0)
                lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
            Hidden_flag = 0;
        }
    }
    return 1;
}

#include "stdio.h"

static uint8_t UI_Get(Task_Parameter_Struct *e)
{
    // mode set
    e->mode = Task_TC;

    e->TC.Counter = 1;

    char Velstr[10] = {0};
    lv_dropdown_get_selected_str(Obj.Vel.dropdown1, Velstr, 10);
    float Vel_value = 0;
    sscanf(Velstr, "%f", &Vel_value);
    if (Vel_value > 360 || Vel_value < -360)
        return 0;

    e->TC.Vel = (Obj.Vel.CW_flag) ? Vel_value : -Vel_value;

    return 1;
}



uint8_t TC_exam_Para_Check(Task_Parameter_Struct* e)
{
    if (e->TC.Counter > 1000 && e->TC.Counter != 0)
        return 0;
    if (e->TC.Vel > 360 || e->TC.Vel < -360)
        return 0;
    return 1;
}
