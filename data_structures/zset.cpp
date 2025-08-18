#include <string.h>

#include "../redis_functions.h"
#include "zset.h"
#include "../utils/common.h"


static bool hcmp(HNode *node, HNode *keynode) {
    ZNode *znode = container_of(node, ZNode, h_node);
    if (znode->key->size != keynode->key->size) {
        return false;
    }
    return memcmp(znode->key, keynode->key->buf, znode->key->size) == 0;
}

static ZNode* new_znode(double score, dstr *key) {
    ZNode *znode = (ZNode*)malloc(sizeof(ZNode));
    avl_init(&znode->avl_node);
    znode->h_node = *new_node(key, T_STR);

    znode->score = score;
    znode->key = dstr_init(key->size);
    dstr_append(&znode->key, key->buf, key->size);

    return znode;
}

static void del_avl_tree(AVLNode *node) {
    if (!node) {
        return;
    }

    ZNode *n = container_of(node, ZNode, avl_node);

    del_avl_tree(node->left);
    del_avl_tree(node->right);

    free(n);
}

// true if the node is smaller than the {score, key} tuple
static bool zless(ZNode *node, double score, dstr *key) {
    if (score != node->score) {
        return node->score < score;
    }
    int ret = memcmp(node->key, key->buf, std::min(key->size, node->key->size));
    if (ret != 0) {
        return ret < 0;
    }
    return node->key->size < key->size;
}

void zset_delete(ZSet *zset, ZNode *znode) {
    // Delete from the hashmap and the tree
    HNode tmp;
    tmp.key = dstr_init(znode->key->size);
    dstr_append(&tmp.key, znode->key->buf, znode->key->size);
    tmp.hcode = znode->h_node.hcode;

    zset->avl_root = avl_del(&znode->avl_node);
    hm_delete(&zset->hmap, &tmp);
    free(znode);
}

// true if insert, false if update
bool zset_insert(ZSet *zset, double score, dstr *key) {
    bool is_insert = true;
    ZNode *node = zset_lookup(zset, key);
    if (node) {
        // Delete and insert again
        zset_delete(zset, node);
        is_insert = false; // not an insert - this is an update / duplicate
    }

    // Create the new node
    ZNode *znode = new_znode(score, key);

    // Insert in the hashmap
    hm_insert(&zset->hmap, &znode->h_node);

    // Insert in the avl tree
    AVLNode **from = &zset->avl_root;
    AVLNode *curr = NULL;
    while (*from) {
        curr = *from;
        ZNode *node = container_of(curr, ZNode, avl_node);
        from = zless(node, score, key) ? &curr->right : &curr->left;
    }

    *from = &znode->avl_node;
    znode->avl_node.parent = curr;
    zset->avl_root = avl_fix(&znode->avl_node);

    return is_insert;
}

ZNode* zset_lookup(ZSet* zset, dstr *key) {
    if (!zset->avl_root) {
        return NULL;
    }

    HNode tmp;
    tmp.key = dstr_init(key->size);
    dstr_append(&tmp.key, key->buf, key->size);
    tmp.hcode = str_hash((uint8_t*)key->buf, key->size);

    // Do a hashmap lookup
    HNode *hnode = hm_lookup(&zset->hmap, &tmp);
    return hnode ? container_of(hnode, ZNode, h_node) : NULL;
}

void zset_clear(ZSet* zset) {
    hm_clear(&zset->hmap);
    del_avl_tree(zset->avl_root);
    zset->avl_root = NULL;
}

ZNode* zset_lower_bound(ZSet *zset, double score, dstr *key) {
    AVLNode **from = &zset->avl_root;
    AVLNode *curr = NULL;
    AVLNode *lb = NULL;

    while (*from) {
        curr = *from;
        ZNode *node = container_of(curr, ZNode, avl_node);
        if (zless(node, score, key)) {
            from = &curr->right;
        }
        else {
            lb = curr;
            from = &curr->left;
        }
    }

    return lb ? container_of(lb, ZNode, avl_node) : NULL;
}

ZNode* zset_offset(ZNode *znode, int32_t offset) {
    AVLNode *node = &znode->avl_node;
    int32_t rank = 0;
    while (rank != offset) {
        if (rank > offset && rank - (int32_t)avl_size(node->left) <= offset) {
            // node is in the left
            node = node->left;
            rank -= (1 + avl_size(node->right));
        }
        else if (rank < offset && rank + (int32_t)avl_size(node->right) >= offset) {
            // node is in the right
            node = node->right;
            rank += (1 + avl_size(node->left));
        }
        else {
            // parent
            AVLNode *parent = node->parent;
            if (!parent) {
                return NULL;
            }
            if (parent->left == node) {
                rank += (1 + avl_size(node->right));
            }
            else {
                rank -= (1 + avl_size(node->left));
            }
            node = parent;
        }
    }

    return node ? container_of(node, ZNode, avl_node) : NULL;
}



