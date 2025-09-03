#include "data_structures/dlist.cpp"
#include "dlist.h"
#include "dstr.h"
#include <cstdio>
#include <assert.h>

void test_dlist_init() {
    DListNode *node = NULL;
    dlist_init(node);
    assert(node->prev == NULL && node->next == NULL);
}

void test_dlist_empty() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *node = NULL;
    node->val = val1;
    dlist_init(node);
    assert(dlist_empty(node));

    DListNode *nxt = NULL;
    nxt->val = val2;
    
    node->next = nxt;
    nxt->prev = node;

    assert(!dlist_empty(node));
}

int run_all_dlist() {
    test_dlist_init();
    test_dlist_empty();
    printf("dlist tests passed!\n");
    return 0;
}
