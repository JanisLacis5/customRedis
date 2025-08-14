#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "dlist.h"
#include "utils/common.h"

struct HNode;
struct ZSet;

struct HTab {
    HNode **tab = NULL;
    size_t mask = 0; // array size is always a power of 2 (n), mask = 2^n-1
    size_t size = 0;
};

struct HMap {
    HTab older;
    HTab newer;
    size_t migrate_pos = 0;
};

struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0; // hash value
    std::string key = "";
    uint32_t type = 100;
    size_t heap_idx = -1;

    // Possible values
    std::string val = "";
    HMap hmap;
    DList list;
    ZSet *zset = NULL;
    HMap hset;
    HMap set;
    std::string bitmap = "";
    std::string hll = "";
};


HNode* new_node(std::string &key, uint32_t type);
HNode* hm_lookup(HMap *hmap, HNode *key);
void hm_insert(HMap *hmap, HNode *node);
HNode* hm_delete(HMap *hmap, HNode *key);
void hm_clear(HMap *hmap);
size_t hm_size(HMap *hmap);
void hm_keys(HMap* hmap, std::vector<std::string> &arg);

#endif
