#include <string.h>

#include "redis_functions.h"
#include "zset.h"

// ptr = pointer to a struct's member
// T = type of the enclosing struct
// member = id for the field in T that ptr points to
#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

struct HKey {
    HNode node;
    std::string name;
    size_t len = 0;
};

// FNV hash
static uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

static bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, h_node);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->key_len != hkey->len) {
        return false;
    }
    return memcmp(znode->key, hkey->name.data(), znode->key_len) == 0;
}

static ZNode* new_znode(double score, std::string &key) {
    ZNode *znode = (ZNode*)malloc(sizeof(ZNode) + key.size());
    avl_init(&znode->avl_node);
    znode->h_node.next = NULL;
    znode->h_node.hcode = str_hash((uint8_t*)key.data(), key.size());

    znode->score = score;
    znode->key_len = key.size();
    memcpy(znode->key, key.data(), key.size());
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
static bool zless(ZNode *node, double score, std::string &key) {
    if (score != node->score) {
        return node->score < score;
    }
    int ret = memcmp(node->key, key.data(), std::min(key.size(), node->key_len));
    if (ret != 0) {
        return ret < 0;
    }
    return node->key_len < key.size();
}

void zset_delete(ZSet *zset, ZNode *znode) {
    // Delete from the hashmap
    HKey hkey;
    hkey.name = znode->key;
    hkey.len = znode->key_len;
    hkey.node.hcode = znode->h_node.hcode;

    hm_delete(&zset->hmap, &hkey.node, hcmp);

    // Delete from the avl tree
    zset->avl_root = avl_del(&znode->avl_node);
    free(znode);
}

// true if insert, false if update
bool zset_insert(ZSet *zset, double score, std::string &key) {
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
        from = zless(node, score, key) ? &curr->left : &curr->right;
    }

    *from = &znode->avl_node;
    znode->avl_node.parent = curr;
    zset->avl_root = avl_fix(&znode->avl_node);

    return is_insert;
}

ZNode* zset_lookup(ZSet* zset, std::string& key) {
    HKey hkey;
    hkey.len = key.size();
    hkey.name = key;
    hkey.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    // Do a hashmap lookup
    HNode *hnode = hm_lookup(&zset->hmap, &hkey.node, &hcmp);
    return hnode ? container_of(hnode, ZNode, h_node) : NULL;
}

void zset_clear(ZSet* zset) {
    hm_clear(&zset->hmap);
    del_avl_tree(zset->avl_root);
    zset->avl_root = NULL;
}

ZNode* zset_lower_bound(ZSet *zset, double score, std::string &key) {
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



