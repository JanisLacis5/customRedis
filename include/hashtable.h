#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 10

struct node {
    char* key;
    char* value;

    struct node* next;
};

struct hashMap {
    int currSize, maxCapacity;

    // pointer to an array of node pointers
    struct node** arr;
};

void setNode(struct node* node, char* key, char* value);
void initHashMap(struct hashMap* map);
unsigned long hash(char* str);
void set(struct hashMap* map, char* key, char* value);
char* get(struct hashMap* map, char* key);
void del(struct hashMap* map, char* key);
int exists(struct hashMap* map, char* key);

#endif