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

    DListNode *n1 = NULL;
    n1->val = val1;
    dlist_init(n1);
    assert(dlist_empty(n1));

    DListNode *n2 = NULL;
    n2->val = val2;
    
    n1->next = n2;
    n2->prev = n1;

    assert(!dlist_empty(n1));
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);
}

void test_dlist_deatach() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = NULL;
    n1->val = val1;
    DListNode *n2 = NULL;
    n2->val = val2;

    n1->next = n2;
    n2->prev = n1;

    dlist_deatach(n2);
    assert(n1->prev == NULL && n1->next == NULL);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);
}

void test_dlist_insert_before() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = NULL;
    n1->val = val1;
    DListNode *n2 = NULL;
    n2->val = val2;

    dlist_insert_before(n1, n2);
    assert(n1->prev == n2);
    assert(n2->next == n1);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);
}

void test_dlist_insert_after() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = NULL;
    n1->val = val1;
    DListNode *n2 = NULL;
    n2->val = val2;

    dlist_insert_after(n1, n2);
    assert(n2->prev == n1);
    assert(n1->next == n2);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);
}

int run_all_dlist() {
    test_dlist_init();
    test_dlist_empty();
    test_dlist_deatach();
    test_dlist_insert_before();
    test_dlist_insert_after();
    printf("dlist tests passed!\n");
    return 0;
}
