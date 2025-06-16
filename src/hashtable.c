#include "../include/hashtable.h"

void setNode(struct Node* node, char* key, char* value) {
    // set key: value
    node->key = key;
    node->value = value;
    
    // set the next node to null
    node->next = NULL;
    return;
}

void initHashMap(struct HashMap* map) {
    map->max_capacity = TABLE_SIZE;
    map->curr_size = 0;

    // allocate memory for an array of size maxCapacity for node pointers
    map->arr = (struct Node**)malloc(sizeof(struct Node*) * map->max_capacity);
}

unsigned long hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % TABLE_SIZE;
}

void set(struct HashMap* map, char* key, char* value) {
    // hash the key
    unsigned long keyIndex = hash(key);

    // create a new node
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    setNode(newNode, key, value);

    if (map->arr[keyIndex] == NULL) {
        // no collision -> add the node at the hashed index
        map->arr[keyIndex] = newNode;
    }
    else {
        // add the current new node at the start of the linked list at index keyIndex
        newNode->next = map->arr[keyIndex];
        map->arr[keyIndex] = newNode;
    }
    map->curr_size++;
}

char* get(struct HashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    // get the node by the hash
    struct Node* keyNode = map->arr[keyIndex];

    // go through the linked list at key keyIndex until the necessary key is found
    while (keyNode != NULL && strcmp(keyNode->key, key) != 0) {
        keyNode = keyNode->next;
    }

    // if key was not found, return null
    if (keyNode == NULL) {
        return NULL;
    } else {
        return keyNode->value;
    }
}

void del(struct HashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    struct Node* keyNode = map->arr[keyIndex];
    struct Node* prevNode = NULL;

    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            if (prevNode != NULL) {
                prevNode->next = keyNode->next;
            } else {
                map->arr[keyIndex] = keyNode->next;
            }
            free(keyNode);
            map->curr_size--;
            break;
        }
        prevNode = keyNode;
        keyNode = keyNode->next;
    }

    return;
}

int exists(struct HashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    struct Node* keyNode = map->arr[keyIndex];

    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            return 1;
        }
        keyNode = keyNode->next;
    }

    return 0;
}
