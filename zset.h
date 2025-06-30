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

void zinsert(ZSet *zset, double &score, std::string &key);
ZNode* zlookup(ZSet *zset, std::string &key);
void zdelete(ZSet *zset, ZNode *znode);

#endif
