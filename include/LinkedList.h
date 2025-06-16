#ifndef LINKEDLIST
#define LINKEDLIST

#include <stdio.h>
#include <stdlib.h>

struct ListNode {
    int data;

    struct ListNode* next;
    struct ListNode* prev;
};

struct LinkedList {
    size_t size;
    struct ListNode* head;
    struct ListNode* tail;
};

void list_initList(struct LinkedList* list);
void list_setNode(struct ListNode* node, int data);
void list_lpush(struct LinkedList* list, struct ListNode* node);
void list_rpush(struct LinkedList* list, struct ListNode* node);
void list_lpop(struct LinkedList* list);
void list_rpop(struct LinkedList* list);
int list_del(struct LinkedList* list, int data);
int list_clear(struct LinkedList* list, int isStackAlloc);
struct ListNode* list_get(struct LinkedList* list, size_t index);

#endif