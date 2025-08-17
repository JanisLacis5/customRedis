#ifndef DLIST_H
#define DLIST_H

#include <cstddef>
#include <stdint.h>
#include <string>
#include "dstr.h"

struct DListNode {
    DListNode *prev = NULL;
    DListNode *next = NULL;
    dstr *val;
};

struct DList {
    DListNode *head = NULL;
    DListNode *tail = NULL;
    uint32_t size = 0;
};

void dlist_init(DListNode *node);
bool dlist_empty(DListNode *node);
void dlist_deatach(DListNode *node);
void dlist_insert_before(DListNode *target, DListNode *node);
void dlist_insert_after(DListNode *target, DListNode *node);


#endif
