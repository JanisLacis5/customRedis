#include <stdlib.h>
#include "unity.h"
#include "linkedList.h"

struct linkedList list;
struct ListNode n1, n2, n3;

void setUp()   {
    list_initList(&list);
}
void tearDown(){
    list_clear(&list, 1);
}

void test_listInitShouldSetHeadTailNullAndSizeZero() {
    TEST_ASSERT_NULL(list.head);
    TEST_ASSERT_NULL(list.tail);
    TEST_ASSERT_EQUAL_size_t(0, list.size);
}

void test_listSetNodeShouldSetDataAndNullifyLinks() {
    list_setNode(&n1, 123);
    TEST_ASSERT_EQUAL_INT(123, n1.data);
    TEST_ASSERT_NULL(n1.next);
    TEST_ASSERT_NULL(n1.prev);
}

void test_listRLPushShouldLinkAllNodesCorrectly() {
    // Set up nodes
    list_setNode(&n1, 10);
    list_setNode(&n2, 20);
    list_setNode(&n3, 30);

    // Push nodes to the list
    list_rpush(&list, &n1);
    list_rpush(&list, &n2);
    list_rpush(&list, &n3);

    TEST_ASSERT_EQUAL_size_t(3, list.size);
    TEST_ASSERT_EQUAL_PTR(&n1, list.head);
    TEST_ASSERT_EQUAL_PTR(&n3, list.tail);

    TEST_ASSERT_NULL(list.head->prev);
    TEST_ASSERT_EQUAL_PTR(&n2, list.head->next);

    TEST_ASSERT_EQUAL_PTR(&n1, n2.prev);
    TEST_ASSERT_EQUAL_PTR(&n3, n2.next);

    TEST_ASSERT_EQUAL_PTR(&n2, list.tail->prev);
    TEST_ASSERT_NULL(list.tail->next);

    TEST_ASSERT_EQUAL_INT(10, list_get(&list, 0)->data);
    TEST_ASSERT_EQUAL_INT(20, list_get(&list, 1)->data);
    TEST_ASSERT_EQUAL_INT(30, list_get(&list, 2)->data);
}

void test_listGetOutOfBoundsReturnsNull() {
    TEST_ASSERT_NULL(list_get(&list, 0));
    list_setNode(&n1, 5);
    list_rpush(&list, &n1);
    TEST_ASSERT_NULL(list_get(&list, 1));
}

void test_listClearEmptyReturnsOne() {
    int ret = list_clear(&list, 1);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_size_t(0, list.size);
}

void test_listClearStackAllocResetsAndReturnsZero() {
    list_setNode(&n1, 5);
    list_rpush(&list, &n1);
    TEST_ASSERT_EQUAL_size_t(1, list.size);

    int ret = list_clear(&list, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_NULL(list.head);
    TEST_ASSERT_NULL(list.tail);
    TEST_ASSERT_EQUAL_size_t(0, list.size);
}

void test_listClearHeapAllocResetsAndReturnsZero(void) {
    // Allocate and init
    struct linkedList* heapList = malloc(sizeof(struct linkedList));
    TEST_ASSERT_NOT_NULL(heapList);
    list_initList(heapList);

    // Create one heap node
    struct ListNode* heapNode = malloc(sizeof(struct ListNode));
    TEST_ASSERT_NOT_NULL(heapNode);

    // Set up the list
    list_setNode(heapNode, 7);
    list_rpush(heapList, heapNode);
    TEST_ASSERT_EQUAL_size_t(1, heapList->size);

    // Clear the list
    int ret = list_clear(heapList, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_NULL(heapList->head);
    TEST_ASSERT_NULL(heapList->tail);
    TEST_ASSERT_EQUAL_size_t(0, heapList->size);

    free(heapList);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_listInitShouldSetHeadTailNullAndSizeZero);
    RUN_TEST(test_listSetNodeShouldSetDataAndNullifyLinks);
    RUN_TEST(test_listRLPushShouldLinkAllNodesCorrectly);
    RUN_TEST(test_listGetOutOfBoundsReturnsNull);
    RUN_TEST(test_listClearEmptyReturnsOne);
    RUN_TEST(test_listClearStackAllocResetsAndReturnsZero);
    RUN_TEST(test_listClearHeapAllocResetsAndReturnsZero);

    return UNITY_END();
}
