#include "task_stroage.h"
#include "../other/list.h"
#include "../other/system_function.h"
#define use_windows
#ifndef ATM32F43_xx
#include "Windows.h"
#include <stdio.h>
#else
#include "stdlib.h"
#include "string.h"
#endif // use_windows

// 定义链表节点结构
typedef struct
{
	int data;
	Task_Parameter_Struct *instant;
	struct Node_Task_Stroage *next;
} Node_Task_Stroage;

// 定义链表结构
typedef struct
{
	Node_Task_Stroage *head;
	int size;
	void (*_free)(void *_Block);
	void *(*_malloc)(size_t size);
} List_Task_Stroage;

//
typedef struct
{
	List_Task_Stroage *list;
} Task_Stroage_Struct;
uint8_t task_change_ready;
Task_Stroage_Struct task;
// find id by instant address
uint8_t Task_Stroage_GetIndexByInstant(Task_Parameter_Struct *info, int *Index);
// 输入输出接口
uint8_t Task_Stroage_Get(Task_Parameter_Struct *info, short INDEX)
{
	Node_Task_Stroage *node = (Node_Task_Stroage *)FindNode((List *)task.list, INDEX);
	if (node == NULL)
		return 0;
	Task_Parameter_Struct *src = node->instant;
	memcpy(info, src, sizeof(Task_Parameter_Struct));
	return 1;
}

uint8_t Task_Stroage_Set(Task_Parameter_Struct src, short INDEX)
{
	Node_Task_Stroage *node = (Node_Task_Stroage *)FindNode((List *)task.list, INDEX);
	if (node == NULL)
		return 0;
	Task_Parameter_Struct *traget = node->instant;
	memcpy(traget, &src, sizeof(Task_Parameter_Struct));
	return 1;
}
// /*
//  读取时调用函数 要自锁
//  写入数据时候要 等函数被调用
// */
// 需要指针保存funtion

// 申请添加
Task_Parameter_Struct *Task_Stroage_add(Task_Parameter_Struct info)
{
	Task_Parameter_Struct *info_pt = task.list->_malloc(sizeof(Task_Parameter_Struct));
	memcpy(info_pt, &info, sizeof(Task_Parameter_Struct));
	Node_Task_Stroage *node = (Node_Task_Stroage *)add((List *)task.list, info_pt);
	ID_UPdata((List *)task.list);
	return info_pt;
}

// 申请插入
Task_Parameter_Struct *Task_Stroage_Insert(Task_Parameter_Struct insert_info, uint16_t insert_index)
{
	List *head = (List *)task.list;
	Node_Task_Stroage *newnode = 0;
	Task_Parameter_Struct *info_pt = head->_malloc(sizeof(Task_Parameter_Struct));
	if (info_pt == 0)
		return 0;
	memcpy(info_pt, &insert_info, sizeof(Task_Parameter_Struct));
	if (insert_index == 0xffff)
	{
		newnode = (Node_Task_Stroage *)add(head, (void *)info_pt);
	}
	else
	{
		newnode = (Node_Task_Stroage *)insert(head, insert_index, (void *)info_pt);
		if (newnode == 0)
			newnode = (Node_Task_Stroage *)add(head, info_pt);
	}
	ID_UPdata(head);
	return info_pt;
}

// 申请删除
uint8_t Task_Stroage_delByID(short ID)
{
	List *head = (List *)task.list;
	if (ID < 0)
		return 0;
	Node *node = FindNode(head, ID);
	if (node == 0)
		return 0;
	if (node->instant != 0)
	{
		removeNode(head, ID);
	}
	return 1;
}

uint8_t Task_Stroage_MoveByID(uint16_t swap1, uint16_t swap2)
{
	List *head = (List *)task.list;
	move(head, swap1, swap2);
	ID_UPdata(head);
	return 1;
}

uint8_t Task_Stroage_delAll(void)
{
	int res = 1;
	while (res)
	{
		res = Task_Stroage_delByID(0);
	}
}

// find id by instant address
uint8_t Task_Stroage_GetIndexByInstant(Task_Parameter_Struct *info, int *Index)
{
	List *head = (List *)task.list;
	Node *node = FindNodeByInstant(head, info);
	if (node == 0)
		return 0;
	int id = node->data;
	*Index = id;
	return 1;
}

uint16_t Task_Stroage_GetArray(Task_Parameter_Struct *taskarray, uint16_t arraySize)
{
	uint16_t ID = 0;
	uint16_t arrayWriteIndex = 0;
	List *head = (List *)task.list;
	if (taskarray == 0 || arraySize == 0)
		return 0;
	while (1)
	{
		Node *node = FindNode(head, ID++);
		if (node == 0)
			break;
		if (node->instant != 0)
		{
			Task_Parameter_Struct *instant = node->instant;
			taskarray[arrayWriteIndex++] = *instant;
		}
	}
	return arrayWriteIndex;
}

int Task_Stroage_GetSize()
{
	List *head = (List *)task.list;
	return head->size;
}

#ifdef use_windows
Task_Parameter_Struct task_info[10];
void Task_Stroage_handle(void)
{
	/* for (int i = 0;i < 5;i++)
	 {
		 task_info[i].mode = Task_VOR;
		 task_info[i].VOR.Counter = i;
		 Node_Task_Stroage* node = add(task.list, &task_info[i]);
		 ID_UPdata(task.list);
	 }*/
	while (1)
	{
		if (task_change_ready)
		{
		}
		ControlDelay(100);
	}
}
#else

#endif //  use_windows

// 初始化
void Task_Stroage_Init()
{
	List *list = (List *)malloc(sizeof(List));
	/// memset(list, 0, sizeof(List));
	List_init(list);
	list->_free = free;
	list->_malloc = malloc;
	task.list = (List_Task_Stroage *)list;
}
