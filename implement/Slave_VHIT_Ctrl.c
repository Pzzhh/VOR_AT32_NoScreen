/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-24 14:44:19
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-17 17:23:47
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\implement\Slave_Vor_Ctrl.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "at32f435_437.h"
#include "Slave_VHIT_Ctrl.h"
#include "motor_para_cal.h"
#include "math.h"
#include "delay.h"
#include "../HARDWARE/SLAVE/Slave1_IO.h"
#include "../HARDWARE/SLAVE/Slave1_Timer.h"

#define PauseResetTick (1)

typedef struct
{
    enum
    {
        running,
        reverse,
        back,
        end,
        pause
    } state;
    uint32_t Tick;
    uint8_t ReqPause;
    uint8_t ReqStop;
    uint8_t ReqNext;
    uint8_t isCW;
    uint16_t ReqCNT;
    uint16_t CurrentCNT;
} VHIT_Machine_parameter;
VHIT_Machine_parameter vhit_para;
#define Pi 3.1415926
// float speeds = 0;
static const float VHIT_ACC_Tick = 100;

void VHIT_Back_init(float tragetPos, float sps, float accMs);
float vel;
static uint8_t motor_set(void)
{
    uint8_t res;
    if (vhit_para.state == running)
    {
        vel = angle_step * VHIT_Vel_Array[vhit_para.Tick];
        vel = (vhit_para.isCW) ? vel : -vel;
        tim_f_sin_set(vel);
        vhit_para.Tick++;
        if (vhit_para.Tick >= 200)
        {
            vhit_para.CurrentCNT++;
            vhit_para.state = reverse;
            return 0;
        }
    }
    if (vhit_para.state == reverse)
    {
        vhit_para.Tick++;
        if (vhit_para.ReqStop)
        {
            vhit_para.state = back;
            vhit_para.ReqStop = 0;
        }
////        if (vhit_para.ReqPause)
////        {
////            vhit_para.ReqPause = 0;
////            vhit_para.state = pause;
////            vhit_para.CurrentCNT = 0;
////        }
        if (vhit_para.ReqNext)
        {
            vhit_para.ReqNext = 0;
            vhit_para.Tick = 0;
            vhit_para.state = (vhit_para.CurrentCNT >= vhit_para.ReqCNT) ? back
                                                                         : running;
        }
        if (vhit_para.CurrentCNT >= vhit_para.ReqCNT)
        {
            vhit_para.state = back;
        }
    }
    if (vhit_para.state == back)
    {
        tim_f_sin_set(0);
        Slave_Release();
        return 1; // END
    }

    //    if (vhit_para.state == pause)
    //    {
    //        Slave_Release();
    //        if (vhit_para.ReqPause)
    //        {
    //            vhit_para.ReqPause = 0;
    //            vhit_para.state = running;
    // #if PauseResetTick
    //            vhit_para.Tick = 0;
    //            vhit_para.CurrentCNT = 0;
    // #endif
    //        }
    //        if (vhit_para.ReqStop)
    //        {
    //            return 1;
    //        }
    //    }
    return 0;
}

static uint8_t VHIT_handler(void)
{
    uint8_t res = 0;
    res = motor_set();
    if (res)
    {
        vhit_para.state = end;
        return 1; // end turn off tim4
    }
    return 0;
}

uint8_t VHIT_Machine_Init(uint16_t ReqCNT, uint8_t isCW)
{
#if 0
    Slave1_Set_Machine_Cb(VHIT_handler);
    vhit_para.Tick = 0;
    vhit_para.state = running;
    vhit_para.MillSecReq = 2000;
#else
    memset(vhit_para, 0, sizeof(vhit_para));
	vhit_para.CurrentCNT=0;
	vhit_para.ReqNext=0;
    vhit_para.ReqCNT = ReqCNT;
    vhit_para.isCW = isCW;
    Slave1_Step_Generator_Init(50000, 10);
    Slave1_Set_Machine_Cb(VHIT_handler);
    vhit_para.state = running;
#endif
    return 1;
}

uint8_t VHIT_Machine_Stop(void)
{
    vhit_para.ReqStop = 1;
    return 0;
}

uint8_t VHIT_Machine_Pause(void)
{
    vhit_para.ReqPause = 1;
    return 0;
}

uint8_t VHIT_Machine_Next(void)
{
    if (vhit_para.state == reverse)
        vhit_para.ReqNext = 1;
    return 0;
}

uint8_t VHIT_Machine_Get_Count(uint32_t *CNTreq, uint32_t *CurrentMillSec)
{
    if (vhit_para.state == end)
        return Imp_finsih;
    if (CNTreq != 0)
        *CNTreq = vhit_para.ReqCNT;
    if (CurrentMillSec != 0)
        *CurrentMillSec = vhit_para.CurrentCNT;
    if (vhit_para.ReqPause == 1)
        return Imp_pausing;
    if (vhit_para.state == pause)
        return Imp_paused;
    return Imp_running;
}

const float VHIT_Vel_Array[201] = {
    0.0671401181328362,
    0.0787266435571731,
    0.0921651003682848,
    0.107724973016865,
    0.125710460380519,
    0.146464228763777,
    0.170371462141849,
    0.197864217263118,
    0.229426088805385,
    0.265597186890224,
    0.306979425865986,
    0.354242119344259,
    0.408127871992306,
    0.469458753526526,
    0.539142734707362,
    0.618180358900804,
    0.707671615951506,
    0.808822977724624,
    0.922954546746783,
    1.05150726095383,
    1.19605008869167,
    1.35828713888967,
    1.54006460182370,
    1.74337742621519,
    1.97037562869881,
    2.22337012207686,
    2.50483793942468,
    2.81742672219528,
    3.16395833218678,
    3.54743143979158,
    3.97102293456122,
    4.43808799902682,
    4.95215868314878,
    5.51694081497150,
    6.13630908326956,
    6.81430013042500,
    7.55510349869587,
    8.36305028063199,
    9.24259933485288,
    10.1983209418842,
    11.2348777913802,
    12.3570032119281,
    13.5694765777794,
    14.8770958532719,
    16.2846472653379,
    17.7968721272041,
    19.4184308720030,
    21.1538643932651,
    23.0075528298371,
    24.9836719752639,
    27.0861475356447,
    29.3186075048669,
    31.6843329713773,
    34.1862077155999,
    36.8266670010583,
    39.6076460044813,
    42.5305283698692,
    45.5960954079025,
    48.8044764943716,
    52.1551012487038,
    55.6466540953934,
    59.2770318264577,
    63.0433047912726,
    66.9416823406611,
    70.9674831443941,
    75.1151109848855,
    79.3780366045036,
    83.7487861493974,
    88.2189367089977,
    92.7791193975023,
    97.4190303619495,
    102.127450031335,
    106.892270843220,
    111.700533599152,
    116.538472508835,
    121.391568886448,
    126.244613361912,
    131.081776366588,
    135.886686548275,
    140.642516665838,
    145.332076411009,
    149.937911505345,
    154.442408325574,
    158.827903222264,
    163.076795616263,
    167.171663886313,
    171.095383000717,
    174.831242797397,
    178.363065780919,
    181.675323283072,
    184.753248825993,
    187.582947534037,
    190.151500462857,
    192.447062751362,
    194.458954554239,
    196.177743778858,
    197.595319730034,
    198.704956858232,
    199.501367910180,
    199.980745894189,
    200.140794394111,
    199.980745894189,
    199.501367910180,
    198.704956858232,
    197.595319730034,
    196.177743778858,
    194.458954554239,
    192.447062751362,
    190.151500462857,
    187.582947534037,
    184.753248825993,
    181.675323283072,
    178.363065780919,
    174.831242797397,
    171.095383000717,
    167.171663886313,
    163.076795616263,
    158.827903222264,
    154.442408325574,
    149.937911505345,
    145.332076411009,
    140.642516665838,
    135.886686548275,
    131.081776366588,
    126.244613361912,
    121.391568886448,
    116.538472508835,
    111.700533599152,
    106.892270843220,
    102.127450031335,
    97.4190303619495,
    92.7791193975023,
    88.2189367089977,
    83.7487861493974,
    79.3780366045036,
    75.1151109848855,
    70.9674831443941,
    66.9416823406611,
    63.0433047912726,
    59.2770318264577,
    55.6466540953934,
    52.1551012487038,
    48.8044764943716,
    45.5960954079025,
    42.5305283698692,
    39.6076460044813,
    36.8266670010583,
    34.1862077155999,
    31.6843329713773,
    29.3186075048669,
    27.0861475356447,
    24.9836719752639,
    23.0075528298371,
    21.1538643932651,
    19.4184308720030,
    17.7968721272041,
    16.2846472653379,
    14.8770958532719,
    13.5694765777794,
    12.3570032119281,
    11.2348777913802,
    10.1983209418842,
    9.24259933485288,
    8.36305028063199,
    7.55510349869587,
    6.81430013042500,
    6.13630908326956,
    5.51694081497150,
    4.95215868314878,
    4.43808799902682,
    3.97102293456122,
    3.54743143979158,
    3.16395833218678,
    2.81742672219528,
    2.50483793942468,
    2.22337012207686,
    1.97037562869881,
    1.74337742621519,
    1.54006460182370,
    1.35828713888967,
    1.19605008869167,
    1.05150726095383,
    0.922954546746783,
    0.808822977724624,
    0.707671615951506,
    0.618180358900804,
    0.539142734707362,
    0.469458753526526,
    0.408127871992306,
    0.354242119344259,
    0.306979425865986,
    0.265597186890224,
    0.229426088805385,
    0.197864217263118,
    0.170371462141849,
    0.146464228763777,
    0.125710460380519,
    0.107724973016865,
    0.0921651003682848,
    0.0787266435571731,
    0.0671401181328362};
