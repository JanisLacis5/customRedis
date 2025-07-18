#include "dlist.h"

void dlist_init(DListNode *node) {
    node->prev = node;
    node->next = node;
}

bool dlist_empty(DListNode *node) {
    return node->next == node;
}

void dlist_deatach(DListNode *node) {
    DListNode *next = node->next;
    DListNode *prev = node->prev;

    if (prev) {
        prev->next = next;
    }
    if (next) {
        next->prev = prev;
    }
}

void dlist_insert_before(DListNode *target, DListNode *node) {
    DListNode *prev = target->prev;
    target->prev = node;
    node->next = target;
    prev->next = node;
    node->prev = prev;
}
