#include "data_structures/dlist.cpp"
#include "dlist.h"
#include "dstr.h"
#include <cstdio>
#include <assert.h>
#include <stdlib.h>

static DListNode* init_node(dstr *val) {
    DListNode *node = (DListNode*)malloc(sizeof(DListNode));
    node->next = NULL;
    node->prev = NULL;
    node->val = val;
    return node;
}

void test_dlist_init() {
    DListNode *node = init_node(NULL);
    dlist_init(node);
    assert(node->prev == node && node->next == node);

    free(node);
}

void test_dlist_empty() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = init_node(val1);
    dlist_init(n1);
    assert(dlist_empty(n1));

    DListNode *n2 = init_node(val2);
    
    n1->next = n2;
    n2->prev = n1;

    assert(!dlist_empty(n1));
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);

    free(n1);
    free(n2);
    free(val1);
    free(val2);
}

void test_dlist_deatach() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = init_node(val1); 
    DListNode *n2 = init_node(val2);

    n1->next = n2;
    n2->prev = n1;

    dlist_deatach(n2);
    assert(n1->prev == NULL && n1->next == NULL);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);

    free(n1);
    free(n2);
    free(val1);
    free(val2);
}

void test_dlist_insert_before() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = init_node(val1);
    DListNode *n2 = init_node(val2);

    dlist_insert_before(n1, n2);
    assert(n1->prev == n2);
    assert(n2->next == n1);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);

    free(n1);
    free(n2);
    free(val1);
    free(val2);
}

void test_dlist_insert_after() {
    dstr *val1 = dstr_init(1);
    dstr *val2 = dstr_init(2);

    DListNode *n1 = init_node(val1);
    DListNode *n2 = init_node(val2);

    dlist_insert_after(n1, n2);
    assert(n1->next == n2);
    assert(n2->prev == n1);
    assert(n1->val->free == 1);
    assert(n2->val->free == 2);

    free(n1);
    free(n2);
    free(val1);
    free(val2);
}

int run_all_dlist() {
    test_dlist_init();
    printf("\t [dlist]: dlist_init() passed! (1/5)\n");
    test_dlist_empty();
    printf("\t [dlist]: dlist_empty() passed! (2/5)\n");
    test_dlist_deatach();
    printf("\t [dlist]: dlist_deatach() passed! (3/5)\n");
    test_dlist_insert_before();
    printf("\t [dlist]: dlist_insert_before() passed! (4/5)\n");
    test_dlist_insert_after();
    printf("\t [dlist]: dlist_insert_after() passed! (5/5)\n");
    return 0;
}
