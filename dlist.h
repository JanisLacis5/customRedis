#ifndef DLIST_H
#define DLIST_H

#include <cstddef>

struct DListNode {
    DListNode *prev = NULL;
    DListNode *next = NULL;
};

void dlist_init(DListNode *node);
bool dlist_empty(DListNode *node);
void dlist_deatach(DListNode *node);
void dlist_insert_before(DListNode *target, DListNode *node);


#endif
