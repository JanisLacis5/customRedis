#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 10

struct Node {
    char* key;
    char* value;

    struct Node* next;
};

struct HashMap {
    int curr_size;
    int max_capacity;

    // pointer to an array of node pointers
    struct Node** arr;
};

void setNode(struct Node* node, char* key, char* value);
void initHashMap(struct HashMap* map);
unsigned long hash(char* str);
void set(struct HashMap* map, char* key, char* value);
char* get(struct HashMap* map, char* key);
void del(struct HashMap* map, char* key);
int exists(struct HashMap* map, char* key);

#endif