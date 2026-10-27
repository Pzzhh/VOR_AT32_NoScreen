#include "Slave1_Timer.h"
#include "Slave1_IO.h"
#include "at32f435_437.h"
#include "math.h"

float acc_angle = 0;
float soft_angle_CNT;
// uint8_t isEnflag = 0;
Slave_Function_CB function_cb;
void TMR1_BRK_TMR9_IRQHandler(void)
{
    if (TMR9->ists_bit.ovfif) // ????????????
    {
        if (function_cb != 0)
        {
            uint8_t res = function_cb();
            if (res)
            {
                function_cb = 0;
                extern void Slave1_En_IO(uint8_t state);
                Slave1_En_IO(0);
                extern void Motor_Set_Speed(float speed);
                extern void Motor_Spd_Pid();
                Motor_Set_Speed(0);
                Motor_Spd_Pid(); // avoid c601 unexpect movement
            }
            Motor_Spd_Pid(); // avoid c601 unexpect movement
        }
        else
        {
            //							Motor_Set_Speed(0);
        }
        acc_angle = Slave1_Get_Encode_Angle();
        //        extern void Motor_Spd_Pid();
        //        Motor_Spd_Pid(); // avoid c601 unexpect movement
        // ��ͣ������
    }
    TMR9->ists = (uint16_t)~TMR_OVF_FLAG;
}
// ��handler����1ʱֹͣ״̬��
void Slave1_Set_Machine_Cb(Slave_Function_CB cb)
{
    Slave1_IO_Init();
    //    Slave1_En_IO(1);
    // Slave1_Step_Generator_Init(50000, 10);
    Slave_server_Init();
    TMR9->ctrl1_bit.tmren = 0;
    function_cb = cb;
    TMR9->ctrl1_bit.tmren = 1;
}

uint8_t Slave1_Get_Machine_state()
{
    if (function_cb != 0)
        return 1;
    else
        return 0;
}

void Step_Phase_Set(u32 arr, u32 psc)
{
    uint32_t precav = TMR1->cval;
    uint32_t prepr = TMR1->pr;
    if (arr == 0 && psc == 0)
    {
        TMR1->ctrl1_bit.tmren = 0;
        return;
    }
    Slave1_En_IO(1);
    tmr_base_init(TMR1, arr, psc);
    TMR1->swevt_bit.ovfswtr = 1;
    TMR1->cval = precav * arr / prepr;
    TMR1->c1dt = arr / 2;
    TMR1->ctrl1_bit.tmren = 1;
}

void SRV1_Encoder_Init()
{
    static uint8_t isinit = 0;
    if (isinit != 0)
        return;
    isinit = 1;
    // const uint32_t ARR_VALUE = 2097156;
    const uint16_t ch1_pin = 5;
    const uint16_t ch2_pin = 3;
    CRM->apb1en_bit.tmr2en = 1;
    CRM->ahben1_bit.gpioben = 1;
    CRM->ahben1_bit.gpioaen = 1;
    // Configure TMR2 in Encoder mode 3 (count on both TI1FP1 and TI2FP2 edges)
    TMR2->ctrl1_bit.pmen = 1;   // 32bit enable
    TMR2->stctrl_bit.smsel = 3; // encoder mode c
    TMR2->cm1_input_bit.c1df = 0;
    TMR2->cm1_input_bit.c2df = 0;
    TMR2->cm1_input_bit.c1c = 2;
    TMR2->cm1_input_bit.c2c = 2;
    tmr_base_init(TMR2, 0xffffffff, 0);
    TMR2->ctrl1_bit.tmren = 1;

    GPIOA->cfgr_bit.iomc5 = 2;
    GPIOB->cfgr_bit.iomc3 = 2;

    GPIOA->muxl |= 0x01 << 20; // set tmr 2 alternate channel
    GPIOB->muxl |= 0x01 << 12;

    TMR2->pr = 1073741;
    TMR2->div = 0;
    TMR2->ctrl1_bit.prben = 1;
    TMR2->ctrl1_bit.tmren = 1;
}

void Slave_server_Init(void)
{

    CRM->apb2en_bit.tmr9en = 1; /// ʹ��TMR9ʱ��

    tmr_cnt_dir_set(TMR9, TMR_COUNT_UP);
    tmr_clock_source_div_set(TMR9, TMR_CLOCK_DIV1);
    tmr_base_init(TMR9, 999, 287); // ��ʼ��TMR9

    tmr_interrupt_enable(TMR9, TMR_OVF_INT, TRUE);

    nvic_irq_enable(TMR1_BRK_TMR9_IRQn, 0x00, 0x00);
    SRV1_Encoder_Init();
}

void Slave1_Step_Generator_Init(uint32_t arr, uint32_t psc)
{
    CRM->apb2en_bit.tmr1en = 1;
    CRM->ahben1_bit.gpioaen = 1;
    TMR1->c1dt = arr / 2;

    GPIOA->cfgr_bit.iomc8 = 2;

    GPIOA->muxh_bit.muxh8 = 1; // PA8
    // Set the prescaler value to have TMR1 counter clock = 1 MHz
    TMR1->div = 84 - 1; // APB2 clock is 84 MHz

    // Set the ARR value to have a PWM frequency of 1 kHz
    TMR1->pr = 1000 - 1;

    // Set the CCR1 value to have a duty cycle of 50%
    TMR1->c1dt = 500;

    // Configure the TMR1 channel 1 in PWM mode 1
    TMR1->cm1_output_bit.c1octrl = 6;

    // Enable the output compare 1 preload register
    TMR1->cm1_output_bit.c1oben = 1;

    // Enable the capture/compare 1 output
    TMR1->cctrl_bit.c1en = 1;

    // Enable auto-reload preload
    // TMR1->ctrl1_bit.prben = 1;

    // Enable the TMR1 main output
    TMR1->brk_bit.oen = 1;

    // Enable the TMR1 counter
    TMR1->ctrl1_bit.tmren = 1;
    TMR1->c1dt = arr / 2;

    Slave_server_Init();
}

void tim_f_set(int f)
{

    tmr_type *TIMx = TMR1;
    u8 Mins_flag;
    int time_arr, time_psc;
    int a, clk = 144000000, c, y, r = 0, t, i;
    if (f == 0)
    {
        tmr_counter_enable(TMR1, FALSE);
        return;
    }
    if (f < 0)
    {
        f = -f;
        Mins_flag = 0;
    }
    else
        Mins_flag = 1;
    a = clk / (f);
    c = sqrt(a);
    y = 10000000;
    for (i = 2; i <= c; i++)
    {
        t = a % (i);

        if (y > t && a / i < 65536)
        {

            y = t;

            r = i;
            if (y == 0 && a / i < 65536)
                break;
        }
    }
    c = 10;

    time_arr = a / r - 1;
    time_psc = r - 1;
    // Out_Put_data = sys_clk / time_arr / time_psc;
    if (f > 1000)
    {
        Slave1_Dir_IO(Mins_flag);
        if (time_arr != TIMx->pr || time_psc != TIMx->div)
            Step_Phase_Set(time_arr, time_psc);
    }
    TIMx->ctrl1 |= 0x0001;
}

void tim_f_sin_set(int f)
{
    tmr_type *TIMx = TMR1;
    float angle = -f;
    soft_angle_CNT += (angle / angle_step) / 1000;
    if (soft_angle_CNT > 180)
    {
        soft_angle_CNT -= 360;
    }
    else if (soft_angle_CNT < -180)
    {
        soft_angle_CNT += 360;
    }
    u8 Mins_flag;
    int time_arr, time_psc;
    int a, clk = 144000000, c, y, r = 0, t, i;
    if (f == 0)
    {
        Step_Phase_Set(0, 0);
        return;
    }
    if (f < 0)
    {
        f = -f;
        Mins_flag = 0;
    }
    else
        Mins_flag = 1;
    // if (f < 10)
    //     return;
    a = clk / (f);
    c = sqrt(a);
    y = 10000000;
    for (i = 2; i <= c; i++)
    {
        t = a % (i);

        if (y > t && a / i < 65536)
        {

            y = t;

            r = i;
            if (y == 0 && a / i < 65536)
                break;
        }
    }
    c = 10;

    time_arr = a / r - 1;
    time_psc = r - 1;
    if (f > 1000 || TIMx->cval > TIMx->c3dt)
    {
        Slave1_Dir_IO(Mins_flag);
        if (time_arr != TIMx->pr || time_psc != TIMx->div)
            Step_Phase_Set(time_arr, time_psc);
    }
    TIMx->ctrl1 |= 0x0001;
}

void tim_f_sin_set_Dir(int f)
{
    tmr_type *TIMx = TMR1;
    u8 Mins_flag;
    int time_arr, time_psc;
    int a, clk = 288000000, c, y, r = 0, t, i;
    if (f == 0)
    {
        Step_Phase_Set(0, 0);
        return;
    }
    if (f < 0)
    {
        f = -f;
        Mins_flag = 0;
    }
    else
        Mins_flag = 1;
    // if (f < 10)
    //     return;
    a = clk / (f);
    c = sqrt(a);
    y = 10000000;
    for (i = 2; i <= c; i++)
    {
        t = a % (i);

        if (y > t && a / i < 65536)
        {

            y = t;

            r = i;
            if (y == 0 && a / i < 65536)
                break;
        }
    }
    c = 10;

    time_arr = a / r - 1;
    time_psc = r - 1;
    Slave1_Dir_IO(Mins_flag);
    if (time_arr != TIMx->pr || time_psc != TIMx->div)
        Step_Phase_Set(time_arr, time_psc);
    TIMx->ctrl1 |= 0x0001;
}

uint8_t Slave_Back(int Tag_Pos)
{
    return 1;
}

void Slave_Release()
{
    Slave1_En_IO(0);
    TMR1->ctrl1_bit.tmren = 0;
}

u8 Slave_Back_spd(int Tag_Pos, int spd)
{
    return 1;
}
u8 slave_sin_back(void)
{
    return 1;
}

int32_t Slave1_Get_Encode()
{
    return (int32_t)TMR2->cval;
}

void Slave1_Set_Encode(uint32_t cnt)
{
    TMR2->cval = cnt;
}

int32_t count_record;
int8_t isEncode_error = 0;
uint8_t Slave1_Check_Encode_Angle(uint8_t isCheck)
{
    // static uint8_t flag = 0;
    int32_t count = *(int32_t *)&(TMR2->cval);

    if (isCheck == 0)
    {
        count_record = count;
        isEncode_error = 0;
        return 0;
    }
    if (isCheck == 1)
    {
        int absvalue = abs(count_record - count);
        if (absvalue < 10)
        {
            isEncode_error = 1;
            return 1;
        }
        return 0;
    }
    return 0;
}

float Slave1_Get_Encode_Angle()
{
	//  return 1;
//    if (isEncode_error)
//    {
       return soft_angle_CNT;
//    }
    //    int32_t count = *(int32_t *)&(TMR2->cval);
    //    count = count - 1073741 / 2;
    float angle = 0;
    angle = Slave1_Get_Encode_Angle_real();
    return angle;
}

float Slave1_Get_Encode_Angle_real()
{

    int32_t count = *(int32_t *)&(TMR2->cval);
    count = count;
    float angle = count;
    angle = angle / 400000 * 360.0f;
    return angle;
}

uint8_t Slave1_get_Encode_error()
{
    return isEncode_error;
}

void Slave1_Set_Encode_Angle(float angle)
{
    soft_angle_CNT = angle;
    angle = angle + 180;

    int32_t *count = (int32_t *)&(TMR2->cval);
    *count = (angle / 360) * 1073741.0f;
}
// void Slave1_CMD(Slave1_CMD_Typed cmd,int freq)
//
