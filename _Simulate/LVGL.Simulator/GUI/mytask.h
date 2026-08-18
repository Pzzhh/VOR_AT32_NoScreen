#ifndef __mytask_h
#define __mytask_h
#include "..\lvgl\lvgl.h"
#include "list_c.h"

int table_get_cnt(int Num);
void Table_btn_init(void);
void Table_list_add(void);
void table_set(int flag, int Num);
void table_hidden(int i);
void table_arrange(void);
int table_get_cnt(int Num);
link *Task_lisk(int num);
void table_init(void);
void Table_reflush(lv_obj_t *table,link *e);
void table_arrange_auto();
void table_usart_Del(int flag, int Num_choose);
int Task_lisk_length();
#define TASK_Num 14
#define list_Create 1
#define list_Delate 2
#define list_save 3

#endif
