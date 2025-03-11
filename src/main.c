#include <stdio.h>
#include "../include/hashtable.h"
#include "../include/linkedList.h"

int main() {
    struct linkedList list;
    list_initList(&list);

    struct ListNode n1, n2, n3, n4;
    list_setNode(&n1, 2);
    list_setNode(&n2, 3);
    list_setNode(&n3, 1);
    list_setNode(&n4, 5);

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

    list_clear(&list);
    printf("%li\n", list.size);


    return 0;
}   