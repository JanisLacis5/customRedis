#include <stdio.h>
#include "../include/hashtable.h"
#include "../include/LinkedList.h"

int main() {
//    struct linkedList* list = malloc(sizeof(struct linkedList));
    struct LinkedList list;
    list_init_list(&list);

//    struct ListNode* n1 = malloc(sizeof(struct ListNode));
//    struct ListNode* n2 = malloc(sizeof(struct ListNode));
//    struct ListNode* n3 = malloc(sizeof(struct ListNode));
//    struct ListNode* n4 = malloc(sizeof(struct ListNode));
    struct ListNode n1, n2, n3, n4;

    list_set_node(&n1, 2);
    list_set_node(&n2, 3);
    list_set_node(&n3, 1);
    list_set_node(&n4, 5);

    list_rpush(&list, &n1);
    list_rpush(&list, &n2);
    list_rpush(&list, &n3);
    list_rpush(&list, &n4);

    struct ListNode* curr = list.head;
    int currId = 0;
    while (curr != NULL) {
        printf("element at index %i: %i\n", currId, list_get(&list, currId)->data);
        curr = curr->next;
        currId++;
    }

    list_clear(&list, 1);
    printf("the list size: %li\n", list.size);


    return 0;
}