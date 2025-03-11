#include "../include/linkedList.h"

void list_initList(struct linkedList* list) {
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    return;
}

void list_setNode(struct ListNode* node, int data) {
    node->data = data;

    node->next = NULL;
    node->prev = NULL;

    return;
}

void list_lpush(struct linkedList* list, struct ListNode* node) {
    if (list->size == 0) {
        list->head = node;
        list->tail = node;
    }
    else {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }

    list->size++;
    return;
}

void list_rpush(struct linkedList* list, struct ListNode* node) {
    if (list->size == 0) {
        list->head = node;
        list->tail = node;
    }
    else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
    return;
}

void list_lpop(struct linkedList* list) {
    if (list->size == 0) {
        return;
    }

    struct ListNode* head = list->head;
    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    }
    else {
        list->head = head->next;
        list->head->prev = NULL;
    }

    list->size--;
    free(head);
    return;
}

void list_rpop(struct linkedList* list) {
    if (list->size == 0) {
        return;
    }

    struct ListNode* tail = list->tail;
    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    }
    else {
        list->tail = tail->prev;
        list->tail->next = NULL;
    }

    list->size--;
    free(tail);
    return;
}

int list_del(struct linkedList* list, int data) {
    if (list->size == 0) {
        return 1;
    }

    struct ListNode* curr = list->head;
    while (curr != NULL && curr->data != data) {
        curr = curr->next;
    }

    if (curr == NULL) {
        printf("element %i not found\n", data);
        return 1;
    }

    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    } 
    else if (curr->prev == NULL) {
        list->head = curr->next; 
        curr->next->prev = NULL;
    } 
    else if (curr->next == NULL) {
        list->tail = curr->prev;
        curr->prev->next = NULL;
    } 
    else {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
    }

    list->size--;
    free(curr);
    return 0;
}

int list_clear(struct linkedList* list) {
    if (list->size == 0) {
        return 1;
    }

    struct ListNode* prev = NULL;
    struct ListNode* curr = list->head;
    while (curr != NULL) {
        prev = curr;
        curr = curr->next;
        free(prev);
    }   

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return 0;
}

struct ListNode* list_get(struct linkedList* list, size_t index) {
    if (list->size == 0) {
        printf("list is empty\n");
        return NULL;
    }

    if (index >= list->size) {
        printf("index %li too large. max index = %li\n", index, list->size-1);
        return NULL;
    }

    struct ListNode* curr;
    size_t currIndex;

    if (index <= list->size / 2) {
        currIndex = 0;
        curr = list->head;
        while (currIndex != index) {
            currIndex++;
            curr = curr->next;
        }
    }
    else {
        currIndex = list->size-1;
        curr = list->tail;
        while (currIndex != index) {
            currIndex--;
            curr = curr->prev;
        }
    }

    return curr;
}
