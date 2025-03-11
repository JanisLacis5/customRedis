#ifndef LINKEDLIST
#define LINKEDLIST

#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int data;

    struct ListNode* next;
    struct ListNode* prev;
};

struct linkedList {
    size_t size;
    struct ListNode* head;
    struct ListNode* tail;
};

void list_initList(struct linkedList* list);
void list_setNode(struct ListNode* node, int data);
void list_lpush(struct linkedList* list, struct ListNode* node);
void list_rpush(struct linkedList* list, struct ListNode* node);
void list_lpop(struct linkedList* list);
void list_rpop(struct linkedList* list);
int list_del(struct linkedList* list, int data);
int list_clear(struct linkedList* list);
struct ListNode* list_get(struct linkedList* list, size_t index);

#endif