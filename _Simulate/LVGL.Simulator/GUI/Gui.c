#include "../GUI/Gui.h"
#include "mytask.h"
//#include "Slave_motor.h"
//#include "led.h"
//#include "wwdg.h"
short Set_time = 1;
lv_obj_t *time_lable;
lv_obj_t *time_box;
volatile struct data1 State = {
    .mode = VOR_ID,
    .task = 0,
    .Vel = 1,
    .Frep_VOR = 10,
    .Set_Time = 1,
    .dir = 1,
};
float Dropdown_read_float(lv_obj_t *dropdown);
short Dropdown_read(lv_obj_t *dropdown);
lv_obj_t *dropdown_1;
short Dropdown_mode = 1;
void start_init(void);
extern lv_timer_t *INC_timer;
extern void cam_timer_on_off(int Period);

/*****************************************************/
#define LV_SYMBOL_rotate_right "\xEF\x80\x9E"
/*Add a border with bg_color*/
#define LV_SYMBOL_rotate_left "\xEF\x83\xA2"
LV_FONT_DECLARE(arrow_rotate_right);
/*****************************************************/

void Btn_Num_sever(lv_event_t *e)
{

    int add = 0;
    add = (int)(e->user_data);
    lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        if (add == 1)
        {
            lv_spinbox_increment(time_box);
            Set_time = lv_spinbox_get_value(time_box);
        }
        else
        {
            lv_spinbox_decrement(time_box);
            Set_time = lv_spinbox_get_value(time_box);
        }
    }
    // lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *obj = lv_event_get_target(e);
    // if (code == LV_EVENT_CLICKED)
    // {
    //     if (e->user_data == (void *)1)
    //     {
    //         if (Set_time < 300)
    //             Set_time += 30;
    //         else
    //             Set_time = 300;
    //         lv_label_set_text_fmt(time_lable, "%ds", Set_time);
    //     }
    //     else
    //     {
    //         if (Set_time > 30)
    //             Set_time -= 30;
    //         else
    //             Set_time = 30;
    //         lv_label_set_text_fmt(time_lable, "%ds", Set_time);
    //     }
    // }
}

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    LV_LOG_USER("Num %d", code);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        switch (buf[0])
        {
        case 'V':
        {
            if (buf[1] == 'O')
                VOR_init(), Dropdown_mode = VOR_ID;
            else
                M4_init(), Dropdown_mode = VHIT_ID;
        }
        break;
        case 'O':
            Ovar_init(), Dropdown_mode = OVAR_ID;
            break;
        case 'C':
            Continue_init(), Dropdown_mode = Ctn_ID;
            break;
        case 'T':
            Tc_init(), Dropdown_mode = TC_ID;
            break;
        }
    }
}

void Menu_init(void)
{
    start_init();
    table_init();
    Table_btn_init();

    /**< 1 */ lv_obj_t *label_0 = lv_label_create(lv_scr_act());
    // 婵☆偆澧楅崹鐟邦啅閿燂拷
    /**< 2 */ dropdown_1 = lv_dropdown_create(lv_scr_act());
    //    lv_obj_set_size(dropdown_1,110,40);
    //    lv_obj_set_size(dropdown_1,130,42);
    /**< 3 */ lv_obj_t *dropdown_1_label = lv_label_create(lv_scr_act());
    lv_obj_align(dropdown_1, LV_ALIGN_LEFT_MID, 110, -120);
    // 闂備緡鍋嗛崰搴ｂ偓鐧告嫹
    // 閻庢鍠掗崑鎾斥攽椤旂⒈鍎愰悗鍨矒閺佹捇鏁撻敓锟??
    static lv_style_t font_style1;
    lv_style_init(&font_style1);
    lv_style_set_text_font(&font_style1, &lv_font_montserrat_18);
    lv_obj_add_style(label_0, &font_style1, 0);

    lv_label_set_text_fmt(label_0, "Mouse vestibular oculomotor reflex test system");
    lv_obj_align(label_0, LV_ALIGN_TOP_MID, 0, 50);
    // 婵☆偆澧楅崹鐟邦啅閿燂拷
    lv_label_set_text_fmt(dropdown_1_label, "Model:");
    lv_obj_align_to(dropdown_1_label, dropdown_1, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_dropdown_set_options(dropdown_1, "VOR\n"
                                        "Continue\n"
                                        "Ovar\n"
                                        "VHIT\n"
                                        "TC");
    //// 闂佽桨绀佹惔婊堝箯閿燂拷?
    lv_obj_add_event_cb(dropdown_1, event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    Model_init();
}
struct
{
    lv_timer_t *e;
    uint8_t State;
} INC_manual_timer;

void INC_manual_timer_handler(void)
{
  /*  inc_up = 0;
    inc_down = 0;*/
    // inc_up = 0;
    // inc_down = 0;
    INC_manual_timer.State = 0;
}

void INC_manual_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    // static int time;
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        if (State.inc_Rec)
        {
            return;
        }
        //if (e->user_data == (void *)1)
        //    // LED0 = 0, LED1 = 1;
        //    //inc_up = 1, inc_down = 0;
        //else
        //    // LED0 = 1, LED1 = 0;
        //    inc_up = 0, inc_down = 1;
        if (INC_manual_timer.State)
        {
            INC_manual_timer.e->period = 200;
            INC_manual_timer.e->last_run = lv_tick_get(); // 濠殿噯绲界换鎴犫偓鍨矋缁嬪鍩€椤掆偓閳诲酣鏁嶉崟顒€鐓曠紓鍌欑贰閸欌偓缂佽绉归弫鎾绘晸閿燂拷?3闂佽法鍣﹂幏锟??
            INC_manual_timer.e->repeat_count = 1;         // 闂佸憡鐟禍锝囨崲瀹ュ洦鍋橀悘鐐跺亹椤忛亶鏌ㄩ悤鍌涘?
        }
        else
        {
            INC_manual_timer.State = 1;
            INC_manual_timer.e = lv_timer_create(INC_manual_timer_handler, 200, 0);
            INC_manual_timer.e->repeat_count = 1;
        }
    }
}



void reset_btn_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    static int32_t time;
    lv_obj_t *label = (lv_obj_t *)e->user_data;
    if (code == LV_EVENT_CLICKED)
    {
        // State.std_ang = State.act_ang;
    }
    if (code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        if (lv_tick_get() - time > 5000)
            time = lv_tick_get();
        // lv_label_set_text_fmt(label,"reset after %ds",6-(lv_tick_get() - time)/1000);
       /* if (lv_tick_get() - time > 4 * 1000)
            WWDG_Init(4, 150, WWDG_Prescaler_8);*/
    }
    else
        lv_label_set_text_fmt(label, " ");
}

void cam_btn_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        //LED1 = 1;
        //cam_timer_on_off(1000);
    }
}




short Dropdown_read(lv_obj_t *dropdown)
{
    char buf[20];
    int i;
    short num = 0;
    memset(buf, 0, 20);
    lv_dropdown_get_selected_str(dropdown, buf, 20);
    for (i = 0; i < 3; i++)
    {
        if (buf[i] <= '9' && buf[i] >= '0')
            num = 10 * num + (buf[i] - 48);
    }
    LV_LOG_USER("\r\n %d", num);
    return num;
}


float Dropdown_read_float(lv_obj_t *dropdown)
{
    char buf[10];
    float num = 0, f = 1;
    int i = 1;
    short flag = 0;
    memset(buf, 0, 10);
    lv_dropdown_get_selected_str(dropdown, buf, 10);
    for (i = 0; i < 5; i++)
    {
        if (buf[i] == '.')
            flag = 1;
        if (buf[i] <= '9' && buf[i] >= '0' && flag == 0)
            num = 10 * num + (buf[i] - 48);
        if (buf[i] <= '9' && buf[i] >= '0' && flag == 1)
        {
            f = f * 0.1;
            num = num + (buf[i] - 48) * f;
        }
    }
//    printf("/r/n drop %f :%s", num, buf);
    return num;
}

static void set_angle(void *obj, int32_t v)
{

    unsigned short i = v - 50;
    if (v < 50)
        i = 0;
    if (v > 720)
        v = 720;
    lv_arc_set_angles(obj, i, v);
}

void wait_slave_ready_ui(void)
{
    lv_obj_t *arc = lv_arc_create(lv_scr_act());
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);  /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE); /*To not allow adjusting by click*/
    lv_obj_center(arc);

    //    lv_arc_set_start_angle(arc,100);
    lv_arc_set_value(arc, 120);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_angle);
    lv_anim_set_time(&a, 8000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE); /*Just for the demo*/
    lv_anim_set_repeat_delay(&a, 0);
    lv_anim_set_values(&a, 0, 770);
    lv_anim_start(&a);
}
// float_int_transfer
int f_int_tran(float k, short num)
{
    int res = k * (num) - (int)k * num;
    if (res < 0)
        res = -res;
    return res;
}
