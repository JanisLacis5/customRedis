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

void set_node(struct Node* node, char* key, char* value);
void init_hash_map(struct HashMap* map);
unsigned long hash(const char* str);
void set(struct HashMap* map, char* key, char* value);
char* get(const struct HashMap* map, const char* key);
void del(struct HashMap* map, const char* key);
int exists(const struct HashMap* map, const char* key);

#endif