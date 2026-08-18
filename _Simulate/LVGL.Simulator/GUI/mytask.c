#include "mytask.h"
//#include "usart.h"
#include "GUI.h"
// struct list_s MY_Task[10];
#define Fn_1 1 // 实现添加时自动定位到第一个选项  包括删除

extern lv_obj_t* dropdown_F_V;
extern lv_obj_t* dropdown_V;
extern lv_obj_t* time_lable;
extern lv_obj_t* time_box;
extern short Set_time;
extern struct data1 State;
link* Task_strat = NULL;
extern float Dropdown_read_float(lv_obj_t* dropdown);
extern short Dropdown_read(lv_obj_t* dropdown);
link* Task_lisk(int num);
void Table_touch_handle(lv_event_t* e);
// extern short Set_time;
extern lv_obj_t* dropdown_1;
short MyListNum; // 目前数
struct
{
    lv_obj_t* Table;
    short cnt;
    short state;
} mylist[15];
short Table_Choose = 0;

static void table_draw_part_event_cb(lv_event_t* e)
{
    int i = 0;
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t* dsc = lv_event_get_param(e);
    /*If the cells are drawn...*/
    if (dsc->part == LV_PART_ITEMS)
    {
        uint32_t row = dsc->id / lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);
        while (obj != Task_lisk(i)->Table)
            i++;
        if (i == Table_Choose)
        {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), dsc->rect_dsc->bg_color, LV_OPA_50);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        if (i == State.task && State.flag != 0)
        {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_YELLOW), dsc->rect_dsc->bg_color, LV_OPA_50);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        if (State.task == Table_Choose && i == Table_Choose)
        {
            if (col == 0)
                dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), lv_color_hex(0XFFFFFF), LV_OPA_60);
            else
                dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_YELLOW), dsc->rect_dsc->bg_color, LV_OPA_50);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
    }
}

void start_init(void);
lv_obj_t* table_box;
void table_init(void)
{
    int i, k;
    table_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(table_box, 400, 300);
    lv_obj_align(table_box, LV_ALIGN_CENTER, 150, 0);
    Task_strat = initLink(15);
    for (i = 0; i < TASK_Num; i++)
    {

        Task_lisk(i)->Table =
            Task_lisk(i)->Table = lv_table_create(table_box);
        lv_obj_set_height(Task_lisk(i)->Table, 50);
        lv_table_set_col_width(Task_lisk(i)->Table, 0, 80);
        for (k = 1; k < 4; k++)
            lv_table_set_col_width(Task_lisk(i)->Table, k, 90);
        lv_obj_add_flag(Task_lisk(i)->Table, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(Task_lisk(i)->Table, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_clear_flag(Task_lisk(i)->Table, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(Task_lisk(i)->Table, 10, NULL);
        lv_obj_add_event_cb(Task_lisk(i)->Table, Table_touch_handle, LV_EVENT_ALL, NULL);
        lv_obj_add_event_cb(Task_lisk(i)->Table, table_draw_part_event_cb, LV_EVENT_ALL, NULL);
        Task_lisk(i)->state = 0;
    }
}

void table_hidden(int i)
{
    if (i >= 1)
        lv_obj_add_flag(Task_lisk(i)->Table, LV_OBJ_FLAG_HIDDEN);
}

void table_show()
{
    int k = 0;
    for (k = 1; k < 12; k++)
    {
        if (Task_lisk(k)->state == 1)
        {
            lv_obj_clear_flag(Task_lisk(k)->Table, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void Dropdown_read_option(lv_obj_t* dropdown, float choose)
{
    //    char buf[20];
    int i;
    short num = 1;
    const char* buf = lv_dropdown_get_options(dropdown);
    for (i = 0; i < 100; i++)
    {
        if (buf[i] == '\n')
            num++;
        if (buf[i] == '\0')
            break;
    }
    for (i = 0; i < num; i++)
    {
        lv_dropdown_set_selected(dropdown, i);
        if (choose == Dropdown_read_float(dropdown))
            break;
    }
}

void task_change_sever(link* temp)
{
    short vel = temp->Vel;
    switch (temp->mode)
    {
    case VOR_ID:
        VOR_init(), Dropdown_mode = VOR_ID, lv_dropdown_set_selected(dropdown_1, VOR_ID - 1);
        break;
    case OVAR_ID:
        Ovar_init(), Dropdown_mode = OVAR_ID, lv_dropdown_set_selected(dropdown_1, OVAR_ID - 1);
        break;
    case Ctn_ID:
        Continue_init(), Dropdown_mode = Ctn_ID, lv_dropdown_set_selected(dropdown_1, Ctn_ID - 1);
        break;
    case VHIT_ID:
        M4_init(), Dropdown_mode = VHIT_ID, lv_dropdown_set_selected(dropdown_1, VHIT_ID - 1);
    case TC_ID:
        Tc_init(), Dropdown_mode = TC_ID, lv_dropdown_set_selected(dropdown_1, TC_ID - 1);
        break;
    }
    Dropdown_read_option(dropdown_F_V, temp->Frep_VOR);
    if (temp->Vel < 0)
        vel = -temp->Vel;
    Dropdown_read_option(dropdown_V, vel);
    Set_time = temp->Set_Time;
    lv_spinbox_set_value(time_box, Set_time);
    if (temp->Vel >= 0)
        State.dir = 1;
    else
        State.dir = 0;
}

void Table_touch_handle(lv_event_t* e)
{
    int i = 0;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        while (target != Task_lisk(i)->Table)
            i++;
        Table_Choose = i;
        lv_obj_align(table_box, LV_ALIGN_CENTER, 150, 0);
        task_change_sever(Task_lisk(Table_Choose));
        LV_LOG_USER("\r\n %d", i);
    }
}

void table_usart_Del(int flag, int Num_choose)
{
   /* const char* mode_text[5] = { "NULL", "VOR", "CON", "OVAR", "VHIT" };
    int i, task_cnt = 0, Num;
    lv_obj_t* table;
    link* e = Task_lisk(0);
    for (i = 1; i < 12; i++)
    {
        if (Task_lisk(i)->state == 1)
            task_cnt++;
        if (Task_lisk(i)->state == 0)
        {
            Num = task_cnt + 1;
            break;
        }
    }
    Task_lisk(Num_choose)->state = 0;
    table_hidden(Num_choose);
    if (Num_choose != 0)
    {
        link_Del(Task_strat, Num_choose);
    }
    table_arrange();
    Table_Choose = 1;
    Table_reflush(table, e);*/
}

void table_set(int flag, int Num_choose)
{
    const char* mode_text[5] = { "NULL", "VOR", "CON", "OVAR", "VHIT" };
    int i, task_cnt = 0, Num;
    lv_obj_t* table;
    link* e;
    for (i = 1; i < 12; i++)
    {
        if (Task_lisk(i)->state == 1)
            task_cnt++;
        if (Task_lisk(i)->state == 0)
        {
            Num = task_cnt + 1;
            break;
        }
    }
    switch (flag)
    {
    case list_Create:
    {
        if (task_cnt == 9)
            return;
        e = Task_lisk(Num);
#if Fn_1
        Table_Choose = Num;
#endif
        // State.Frep_VOR = Dropdown_read_float(dropdown_F_V);
        // State.Vel = Dropdown_read(dropdown_V);
        // State.Set_Time = Set_time;
        e->state = 1;
        table_show();
        // table_arrange();
        e->Frep_VOR = Dropdown_read_float(dropdown_F_V);
        //        printf("\r\n frep %f", e->Frep_VOR);
        e->mode = Dropdown_mode;
        e->Vel = Dropdown_read(dropdown_V);
        if (State.dir == 0 && (e->mode == Ctn_ID || e->mode == OVAR_ID || e->mode == TC_ID)) // 方向反转
            e->Vel = -e->Vel;
        e->Set_Time = Set_time;
        table = e->Table;
        LV_LOG_USER("\r\n %d", Dropdown_mode);
    }
    break;
    case list_Delate:
    {
        // #if Fn_1
        // if(Table_Choose==Num-1&&Table_Choose>0)
        //     Table_Choose--;
        // // if(Num-2>=1)
        // //     Table_Choose =Num-2;
        // #endif
        Task_lisk(Table_Choose)->state = 0;
        table_hidden(Table_Choose);
        if (Table_Choose != 0)
        {
            link_Del(Task_strat, Table_Choose);
        }

        Table_Choose = 1;
        // table_arrange();
        return;
    }
    break;
    case list_save:
    {
        if (State.flag == 1)
            return;
        // LV_LOG_USER("\r\n save %d %f", State.Vel, State.Frep_VOR);
        e = Task_lisk(Table_Choose);
        table = Task_lisk(Table_Choose)->Table;
        e->Frep_VOR = Dropdown_read_float(dropdown_F_V);
        e->mode = Dropdown_mode;
        e->Vel = Dropdown_read(dropdown_V);
        if (State.dir == 0 && (e->mode == Ctn_ID || e->mode == OVAR_ID || e->mode == TC_ID)) // 方向反转
            e->Vel = -e->Vel;
        e->Set_Time = Set_time;
        // Task_lisk(Table_Choose)->Frep_VOR = State.Frep_VOR;
        // Task_lisk(Table_Choose)->mode = Dropdown_mode;
        // Task_lisk(Table_Choose)->Vel = State.Vel;
        // Task_lisk(Table_Choose)->Set_Time = State.Set_Time;
    }
    break;
    }
    Table_reflush(table, e);
}

void Table_reflush(lv_obj_t* table, link* e)
{
    const char* mode_text[] = { "NULL", "VOR", "CON", "OVAR", "VHIT", "TC" };
    lv_table_set_cell_value(table, 0, 0, mode_text[e->mode]);
    lv_table_set_cell_value(table, 0, 1, " ");
    lv_table_set_cell_value(table, 0, 2, " ");
    lv_table_set_cell_value_fmt(table, 0, 3, " ");
    switch (e->mode)
    {
    case VOR_ID:
    {
        lv_table_set_cell_value_fmt(table, 0, 2, "%d ", e->Vel);
        lv_table_set_cell_value_fmt(table, 0, 3, "%d.%dHZ", (int)e->Frep_VOR, (int)(e->Frep_VOR * 10 - (int)e->Frep_VOR * 10));
        lv_table_set_cell_value_fmt(table, 0, 1, "%d T", e->Set_Time);
    }
    break;
    case Ctn_ID:
    {
        lv_table_set_cell_value_fmt(table, 0, 2, "%d", e->Vel);
        lv_table_set_cell_value_fmt(table, 0, 1, "%d s", e->Set_Time);
    }
    break;
    case OVAR_ID:
    {
        lv_table_set_cell_value_fmt(table, 0, 2, "%d", e->Vel);
        lv_table_set_cell_value_fmt(table, 0, 1, "%d s", e->Set_Time);
        lv_table_set_cell_value_fmt(table, 0, 3, "%d.%dd", (int)e->Frep_VOR, (int)(e->Frep_VOR * 10 - (int)e->Frep_VOR * 10));
    }
    break;
    case VHIT_ID:
    {
        lv_table_set_cell_value_fmt(table, 0, 1, "%d T", e->Set_Time);
    }
    break;
    case TC_ID:
    {
        lv_table_set_cell_value_fmt(table, 0, 1, "%d s ", e->Set_Time);
        lv_table_set_cell_value_fmt(table, 0, 2, "%d", e->Vel);
    }
    break;
    default:
        break;
    }
}

// 排列
void table_arrange()
{
    int i;
    for (i = 1; i < 12; i++)
    {
        if (Task_lisk(i)->state == 1)
        {
            if (i >= 1)
                lv_obj_align(Task_lisk(i)->Table, LV_ALIGN_DEFAULT, 0, 60 * (i - 1));
        }
    }
}
// 排列动画
void table_arrange_auto()
{
    int i;
    // static short obj_pos[15];
    float miu = 0;
    for (i = 1; i < 12; i++)
    {
        if (Task_lisk(i)->state == 1)
        {
            if (i >= 1)
            {
                miu = 60 * (i - 1) - Task_lisk(i)->pos;
                if (Task_lisk(i)->pos < 60 * (i - 1) - 2 || Task_lisk(i)->pos > 60 * (i - 1) + 2)
                {
                    if (miu > 10 || miu < -10)
                        miu = miu * 0.2;
                    else if (miu < 0)
                        miu = -2;
                    else
                        miu = 2;
                    Task_lisk(i)->pos = Task_lisk(i)->pos + miu;

                    // Task_lisk(i)->pos/=;
                    lv_obj_align(Task_lisk(i)->Table, LV_ALIGN_DEFAULT, 0, Task_lisk(i)->pos);
                }
            }
        }
        else
            Task_lisk(i)->pos = 0;
    }
}

void Table_list_add()
{
    int i;
    for (i = 0; i < TASK_Num; i++)
    {
        if (MyListNum)
            ;
    }
}

void Table_move(int i)
{
    i = i - 1;
    link_Remove(Task_lisk(0), i, i + 1);
    table_arrange();
}

void link_re(int i)
{
    link* p = Task_lisk(i);
    Task_lisk(i - 1)->next = Task_lisk(i)->next;
    p = Task_lisk(i)->next;
}

void Table_btn_handler(lv_event_t* e)
{
    int temp = (int)e->user_data;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && State.flag == 0) // 启动后链表按键不可动
    {
        switch (temp)
        {
        case 0:
        {

            table_set(list_Create, 1);
        }
        break;
        case 1:
        {
            table_set(list_Delate, 1);
        }
        break;
        case 3:
        {
            if (Task_lisk(Table_Choose)->next->state != 0)
            {
                LV_LOG_USER("\r\n now %d", Table_Choose);
                Table_move(Table_Choose + 1);
                Table_Choose++;
            }
        }
        break;
        case 4:
        {
            table_set(list_save, Table_Choose);
        }
        break;
        }
    }
}

void Table_btn_init(void)
{
    /**< 加计划 */
    lv_obj_t* add_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(add_btn, 50, 50);

    lv_obj_set_style_bg_img_src(add_btn, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(add_btn, Table_btn_handler, LV_EVENT_ALL, 0);
    // end
    /**< 减计划 */
    lv_obj_t* Trash_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(Trash_btn, 50, 50);
    lv_obj_align_to(Trash_btn, table_box, LV_ALIGN_OUT_BOTTOM_MID, -30, 20);
    lv_obj_align_to(add_btn, Trash_btn, LV_ALIGN_OUT_LEFT_MID, -20, 0);

    lv_obj_set_style_bg_img_src(Trash_btn, LV_SYMBOL_TRASH, 0);
    lv_obj_add_event_cb(Trash_btn, Table_btn_handler, LV_EVENT_ALL, (void*)1);
    // end
    /**< 移动 */
    lv_obj_t* move_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(move_btn, 50, 50);
    lv_obj_align_to(move_btn, Trash_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_set_style_bg_img_src(move_btn, LV_SYMBOL_DOWN, 0);
    lv_obj_add_event_cb(move_btn, Table_btn_handler, LV_EVENT_ALL, (void*)3);
    // end
    /**< 保存 */
    lv_obj_t* save_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(save_btn, 50, 50);
    lv_obj_align_to(save_btn, move_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_set_style_bg_img_src(save_btn, LV_SYMBOL_SAVE, 0);
    lv_obj_add_event_cb(save_btn, Table_btn_handler, LV_EVENT_ALL, (void*)4);
}

link* Task_lisk(int num)
{
    int i;
    link* temp = Task_strat;
    if (num < 0)
        return temp;
    for (i = 0; i < num; i++)
    {
        if (temp->next != NULL)
            temp = temp->next;
    }
    return temp;
}

int Task_lisk_length()
{
    int i, k;
    link* temp = Task_strat;
    k = 0;
    for (;;)
    {
        if (temp->next != NULL)
        {
            if (temp->next->state == 0)
            {
                break;
            }
        }
        temp = temp->next;
        k++;
    }
    return k; //长度
}
