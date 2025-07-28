#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0; // hash value
    std::string key = "";
    std::string val = "";
};

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

HNode* hm_lookup(HMap *hmap, HNode *key);
void hm_insert(HMap *hmap, HNode *node);
HNode* hm_delete(HMap *hmap, HNode *key);
void hm_clear(HMap *hmap);
size_t hm_size(HMap *hmap);
void hm_keys(HMap* hmap, std::vector<std::string> &arg);

#endif
