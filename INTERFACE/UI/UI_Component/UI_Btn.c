#include "UI_Btn.h"
#include "../UI_Include.h"


lv_obj_t* OperationTogglerBtn_label;
lv_obj_t* OperationTogglerBtn;
uint8_t UI_OperationTogglerBtn_Clicked;

uint8_t UI_OperationTogglerBtn_isClicked()
{
    if (UI_OperationTogglerBtn_Clicked)
    {
        UI_OperationTogglerBtn_Clicked = 0;
        return 1;
    }
    return 0;
}

static void UI_OperationTogglerBtn_Handler(lv_event_t* e)
{
    lv_obj_t* btn_last = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* label = lv_obj_get_child(btn_last, 0);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_t* dropdown1 = lv_obj_get_child(lv_scr_act(), 2);
        lv_obj_t* dropdown2 = lv_obj_get_child(lv_scr_act(), 4);
        UI_OperationTogglerBtn_Clicked = 1;

    }
}


void UI_OperationToggleBtn_change_Display(btn_Display_Tpye_Enum id, int Num)
{
    //    int temp= lv_tick_get()-start_time,tempb =State.Set_Time ;
    switch (id)
    {
    case bt_DSP_stop:
    {
        lv_label_set_text_fmt(OperationTogglerBtn_label, "#000000  Stop#");
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0XFFF000), 0); // 闁跨噦鎷�??
    }
    break;
    case bt_DSP_wait_inc:
    {
        lv_label_set_text(OperationTogglerBtn_label, "#000000 Wait#\n #000000 INC#");
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0xFFFF00), 0); // 闁跨噦鎷�??
    }
    break;
    case bt_DSP_wait_back:
    {
        lv_label_set_text(OperationTogglerBtn_label, "#000000 Wait#");
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0xFFFF00), 0); // 闁跨噦鎷�??
    }
    break;
    case bt_DSP_start:
    {
        lv_label_set_text(OperationTogglerBtn_label, "#000000 Start#");
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0x7CFC00), 0); // 缂備浇鍎绘竟锟�
    }
    break;
    case bt_DSP_next:
    {
        lv_label_set_text_fmt(OperationTogglerBtn_label, "#000000 %ds REC\r#000000   stop#", Num);
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0XFFF000), 0); // 缂備浇鍎绘竟锟�
    }
    break;
    case bt_DSP_run:
    {
        lv_label_set_text_fmt(OperationTogglerBtn_label, "#000000  Stop#");
        lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0XFF0000), 0); // 缂備浇鍎绘竟锟�
    }
    break;

    default:
        break;
    }
}



void UI_OperationToggleBtn_init(lv_obj_t* parent)
{
    /**< 11 */ OperationTogglerBtn = lv_btn_create(parent);

    OperationTogglerBtn_label = lv_label_create(OperationTogglerBtn);
    lv_label_set_recolor(OperationTogglerBtn_label, 1);
    lv_label_set_text(OperationTogglerBtn_label, "#000000 Start#");
    lv_obj_set_size(OperationTogglerBtn, 80, 80);
    lv_obj_set_style_radius(OperationTogglerBtn, 35, 0);
    lv_obj_align(OperationTogglerBtn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(OperationTogglerBtn, LV_ALIGN_TOP_RIGHT, -50, 0);
    lv_obj_set_style_bg_color(OperationTogglerBtn, lv_color_hex(0x7CFC00), 0);
    lv_obj_add_event_cb(OperationTogglerBtn, UI_OperationTogglerBtn_Handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(OperationTogglerBtn, UI_OperationTogglerBtn_Handler, LV_EVENT_DRAW_MAIN_BEGIN, NULL);

}
