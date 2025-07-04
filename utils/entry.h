#ifndef ENTRY_H
#define ENTRY_H

#include "../data_structures/zset.h"
#include "../data_structures/hashmap.h"

enum EntryTypes {
    T_STR = 0,
    T_ZSET = 1
};

struct Lookup {
    HNode node;
    std::string key;
};

struct Entry {
    HNode node;
    std::string key;
    uint32_t type = 2;

    // One of the variants
    std::string value;
    ZSet zset;

    // For ttl heap
    size_t heap_idx = -1;
};

void entry_del(Entry *entry);
bool entry_eq(HNode *node, HNode *key);
Entry* new_entry(std::string &key, uint32_t type);
void entry_del(Entry* entry);


#endif
