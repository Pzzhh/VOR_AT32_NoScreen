#ifndef Gui_h
#define Gui_h

#include "../lvgl/lvgl.h"
void Menu_init(void);

// typedef enum
// {
//     flag_idle,
//     flag_inc,
//     flag_

// }
typedef enum 
{
    bt_stop = 1,
    bt_wait_inc,
    bt_wait_back,
    bt_start = 4,
    bt_next,
    bt_run,
}btn_id;

struct data1
{
    //按键状态
    uint8_t flag;
    //频率和倾角（公用）
    float Frep_VOR;
    //角速度
    short Vel;
    //设置的时间
    int Set_Time;
    //设置的模式
    short mode;
    //开始时间
    int time_start;
    //电机运转状态
    short motor_run;
    //倾角启动
    short inc_Rec;
    //倾角当前值
    float inc_ang;
    //当前任务 与启动一起被置位0
    short task;
    //链表状态
    short list_state;
    //启动角度
    float std_angA;
    float std_angB;
    float real_angA;
    float real_angB;
    //实际角度
    float act_ang;
    //方向设置
    short dir;
    //按键状态
    uint8_t btn_clicked;
    uint8_t uart_cmd;
    uint8_t cam_state;
    uint8_t run;

};

void Model_init(void);
void Ovar_init(void);
void Continue_init(void);
void VOR_init(void);
void start_btn_flash(void);
void start_btn_change(btn_id id, int Num);
void M4_init(void);
void Tc_init(void);
void wait_slave_ready_ui(void);
int f_int_tran(float k, short num);
#define VOR_ID 1
#define Ctn_ID 2
#define OVAR_ID 3
#define VHIT_ID 4
#define TC_ID 5



extern short Dropdown_mode;

extern lv_timer_t *Start_timer;
extern lv_timer_t *flag_sev_timer;
extern lv_timer_t *END_timer;
extern lv_timer_t *NEXT_timer;
extern lv_timer_t *INC_timer;
extern lv_timer_t *Motor_timer;
#define btn_reset_val State.btn_clicked=0;

#endif // Gui_h
