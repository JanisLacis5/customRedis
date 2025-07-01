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
    size_t key_len = 0;
    char key[0];
};

bool zset_insert(ZSet *zset, double score, std::string &key);
ZNode* zset_lookup(ZSet *zset, std::string &key);
void zset_delete(ZSet *zset, ZNode *znode);
void zset_clear(ZSet *zset);
ZNode* zset_lower_bound(ZSet *zset, double score, std::string &key);
ZNode* zset_offset(ZNode *znode, uint32_t offset);

#endif
