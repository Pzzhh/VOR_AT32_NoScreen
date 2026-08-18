#ifndef __list_c_h
#define __list_c_h
#include "list_c.h"
#include  "..\lvgl\lvgl.h"



typedef struct Link
{
    lv_obj_t * Table;
    short cnt;
    short state;
    short Vel;
    float Frep_VOR;
    short Set_Time;
    short mode;
    struct Link *next;
    short pos;
} link;

void display(link *p);
link * initLink(int num);
link* link_find(link *p,int num);
void link_Remove(link *p,int cnt,int TO_cnt);
void link_Del(link *p, int cnt);


#endif
