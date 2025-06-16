#include "../include/hashtable.h"

void set_node(struct Node* node, char* key, char* value) {
    // set key: value
    node->key = key;
    node->value = value;

    // set the next node to null
    node->next = NULL;
}

void init_hash_map(struct HashMap* map) {
    map->max_capacity = TABLE_SIZE;
    map->curr_size = 0;

    // allocate memory for an array of size maxCapacity for node pointers
    map->arr = (struct Node**)malloc(sizeof(struct Node*) * map->max_capacity);
}

unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = (hash << 5) + hash + c; /* hash * 33 + c */

    return hash % TABLE_SIZE;
}

void set(struct HashMap* map, char* key, char* value) {
    // hash the key
    const unsigned long key_index = hash(key);

    // create a new node
    struct Node* new_node = malloc(sizeof(struct Node));
    set_node(new_node, key, value);

    if (map->arr[key_index] == NULL) {
        // no collision -> add the node at the hashed index
        map->arr[key_index] = new_node;
    }
    else {
        // add the current new node at the start of the linked list at index key_index
        new_node->next = map->arr[key_index];
        map->arr[key_index] = new_node;
    }
    map->curr_size++;
}

char* get(const struct HashMap* map, const char* key) {
    const unsigned long key_index = hash(key);

    // get the node by the hash
    const struct Node* key_node = map->arr[key_index];

    // go through the linked list at key key_index until the necessary key is found
    while (key_node != NULL && strcmp(key_node->key, key) != 0) {
        key_node = key_node->next;
    }

    // if key was not found, return null
    if (key_node == NULL) {
        return NULL;
    }
    return key_node->value;
}

void del(struct HashMap* map, const char* key) {
    const unsigned long key_index = hash(key);

    struct Node* key_node = map->arr[key_index];
    struct Node* prevNode = NULL;

    while (key_node != NULL) {
        if (strcmp(key_node->key, key) == 0) {
            if (prevNode != NULL) {
                prevNode->next = key_node->next;
            } else {
                map->arr[key_index] = key_node->next;
            }
            free(key_node);
            map->curr_size--;
            break;
        }
        prevNode = key_node;
        key_node = key_node->next;
    }
}

int exists(const struct HashMap* map, const char* key) {
    const unsigned long key_index = hash(key);
    const struct Node* key_node = map->arr[key_index];

    while (key_node != NULL) {
        if (strcmp(key_node->key, key) == 0) {
            return 1;
        }
        key_node = key_node->next;
    }

    return 0;
}
