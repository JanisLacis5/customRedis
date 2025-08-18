#ifndef ZSET_H
#define ZSET_H

#include "avl_tree.h"
#include "hashmap.h"

struct ZSet {
    AVLNode *avl_root = NULL;
    HMap hmap;
};

struct ZNode {
    AVLNode avl_node;
    HNode h_node;

    double score = 0;
    dstr *key;
};

bool zset_insert(ZSet *zset, double score, dstr *key);
ZNode* zset_lookup(ZSet *zset, dstr *key);
void zset_delete(ZSet *zset, ZNode *znode);
void zset_clear(ZSet *zset);
ZNode* zset_lower_bound(ZSet *zset, double score, dstr *key);
ZNode* zset_offset(ZNode *znode, int32_t offset);

#endif
