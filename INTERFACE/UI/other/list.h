/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @Date: 2024-07-22 16:00:07
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com.
 * @LastEditTime: 2024-07-23 15:56:20
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\other\list.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LIST_H
#define LIST_H
#include "stdio.h"

// 定义链表节点结构
typedef struct Node {
    int data;
    void* instant;
    struct Node* next;
} Node;

// 定义链表结构
typedef struct List {
    Node* head;
    int size;
    void(*_free)(void* _Block);
    void* (*_malloc)(size_t size);
} List;

// 创建新的链表
void List_init(List* list);

// 销毁链表
void destroyList(List* list);

// 添加节点到链表末尾
Node* add(List* list, void* Instant);

// 从链表中移除节点
void removeNode(List* list, int INDEX);

// 在指定位置插入节点
Node* insert(List* list, int index, void* Instant);

// 移动节点
void move(List* list, int fromIndex, int toIndex);

// 打印链表
void printList(List* list);

Node* FindNode(List* list, short INDEX);

//find node by instant
Node* FindNodeByInstant(List* list, void* Instatnt);

// delete node by instant
void DelNodeByInstant(List* list, void* Instatnt);

void ID_UPdata(List* list);

#endif // LIST_H
