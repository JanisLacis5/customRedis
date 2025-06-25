#include "hashmap.h"

HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {

}

void hm_insert(HMap* hmap, HNode* node) {

}

HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {

}

void hm_clear(HMap* hmap) {

}

size_t hm_size(HMap* hmap) {
    return hmap->newer.size() + hmap->older.size();
}




