#include <stdio.h>
#include <stdlib.h>
#include "list.h"

// 创建新的链表
void List_init(List* list) {
    //List* list = (List*)list->_malloc(sizeof(List));
    list->head = NULL;
    list->size = 0;
    return ;
}

// 销毁链表
void destroyList(List* list) {
    Node* current = list->head;
    Node* next;
    while (current != NULL) {
        next = current->next;
        list->_free(current);
        current = next;
    }
    list->_free(list);
}

// 创建新的节点
Node* createNode(List* list, int data) {
    Node* newNode = (Node*)list->_malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// 添加节点到链表末尾
Node* add(List* list, void* Instant) {
    int counter = 0;
    Node* newNode = createNode(list, 0);
    if (list->head == NULL) {
        list->head = newNode;
    }
    else {
        Node* temp = list->head;
        counter = 1;
        while (temp->next != NULL) {
            temp = temp->next;
            counter++;
        }
        temp->next = newNode;
    }
    newNode->data = counter;
    newNode->instant = Instant;
    list->size++;
    return newNode;
}

// 从链表中移除节点
void removeNode(List* list, int INDEX) {
    if (list->head == NULL || INDEX < 0) return;

    Node* temp = list->head;
    Node* prev = NULL;
    int counter = 0;
    // 如果头节点是要移除的节点
    if (temp != NULL && INDEX == 0) {
        list->head = temp->next;
        list->_free(temp);
        list->size--;
        return;
    }

    // 找到要移除的节点
    while (temp != NULL && counter != INDEX) {
        prev = temp;
        temp = temp->next;
        counter++;
    }

    // 如果找不到该节点
    if (temp == NULL) return;

    // 从链表中移除节点
    prev->next = temp->next;
    list->_free(temp);
    list->size--;
}

// 在指定位置插入节点 在a之前插入id
Node* insert(List* list, int index, void* Instant) {
    if (index < 0 || index > list->size) return NULL;

    Node* newNode = createNode(list, 0);

    if (index == 0) {
        newNode->next = list->head;
        list->head = newNode;
    }
    else {
        Node* temp = list->head;
        for (int i = 0; i < index - 1; i++) {
            temp = temp->next;
        }
        newNode->next = temp->next;
        temp->next = newNode;
    }
    newNode->instant = Instant;
    list->size++;
    return newNode;
}

// 移动节点 把a移动到b之后
void move(List* list, int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= list->size || toIndex < 0 || toIndex >= list->size || fromIndex == toIndex) return;

    Node* temp = list->head;
    Node* prev = NULL;
    Node* moveNode = NULL;

    // 找到fromIndex节点
    for (int i = 0; i < fromIndex; i++) {
        prev = temp;
        temp = temp->next;
    }
    moveNode = temp;

    // 从链表中移除该节点
    if (prev != NULL) {
        prev->next = temp->next;
    }
    else {
        list->head = temp->next;
    }

    // 插入该节点到toIndex位置
    if (toIndex == 0) {
        moveNode->next = list->head;
        list->head = moveNode;
    }
    else {
        temp = list->head;
        for (int i = 0; i < toIndex - 1; i++) {
            temp = temp->next;
        }
        moveNode->next = temp->next;
        temp->next = moveNode;
    }
}

// 移动节点
Node* FindNode(List* list, short INDEX) {
    Node* temp = list->head;
    // Node* next = NULL;
    short Counter = 0;
    while (temp != NULL)
    {
        if (temp == NULL)
            return NULL;
        if (Counter == INDEX)
            return temp;
        temp = temp->next;
        Counter++;
    }
    return NULL;
}

// 移动节点
Node* FindNodeByInstant(List* list, void* Instatnt) {
    Node* temp = list->head;
    // Node* next = NULL;
    // short Counter = 0;
    while (temp != NULL)
    {
        if (temp == NULL)
            return NULL;
        if (temp->instant == Instatnt)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

// 
void DelNodeByInstant(List* list, void* Instatnt) {
    if (list->head == NULL || Instatnt == 0) return;

    Node* temp = list->head;
    Node* prev = NULL;
    int counter = 0;
    // 如果头节点是要移除的节点
    if (temp != NULL && Instatnt == temp->instant) {
        list->head = temp->next;
        list->_free(temp);
        list->size--;
        return;
    }

    // 找到要移除的节点
    while (temp != NULL && temp->instant != Instatnt) {
        prev = temp;
        temp = temp->next;
        counter++;
    }

    // 如果找不到该节点
    if (temp == NULL) return;

    // 从链表中移除节点
    prev->next = temp->next;
    list->_free(temp);
    list->size--;
}

void ID_UPdata(List* list)
{
    Node* temp = list->head;
    // Node* next = NULL;
    int Counter = 0;
    while (temp != NULL)
    {
        temp->data = Counter;
        temp = temp->next;
        Counter++;

    }
}

// 打印链表
void printList(List* list) {
    Node* temp = list->head;
    while (temp != NULL) {
        printf("%d -> ", temp->data);
        temp = temp->next;
    }
    printf("NULL\n");
}
