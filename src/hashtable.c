#include "../include/hashtable.h"

void setNode(struct node* node, char* key, char* value) {
    // set key: value
    node->key = key;
    node->value = value;
    
    // set the next node to null
    node->next = NULL;
    return;
}

void initHashMap(struct hashMap* map) {
    map->maxCapacity = TABLE_SIZE;
    map->currSize = 0;

    // allocate memory for an array of size maxCapacity for node pointers
    map->arr = (struct node**)malloc(sizeof(struct node*) * map->maxCapacity);
}

unsigned long hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % TABLE_SIZE;
}

void set(struct hashMap* map, char* key, char* value) {
    // hash the key
    unsigned long keyIndex = hash(key);

    // create a new node
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
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
    map->currSize++;
}

char* get(struct hashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    // get the node by the hash
    struct node* keyNode = map->arr[keyIndex];

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

void del(struct hashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    struct node* keyNode = map->arr[keyIndex];
    struct node* prevNode = NULL;

    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            if (prevNode != NULL) {
                prevNode->next = keyNode->next;
            } else {
                map->arr[keyIndex] = keyNode->next;
            }
            free(keyNode);
            map->currSize--;
            break;
        }
        prevNode = keyNode;
        keyNode = keyNode->next;
    }

    return;
}

int exists(struct hashMap* map, char* key) {
    unsigned long keyIndex = hash(key);

    struct node* keyNode = map->arr[keyIndex];

    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            return 1;
        }
        keyNode = keyNode->next;
    }

    return 0;
}
