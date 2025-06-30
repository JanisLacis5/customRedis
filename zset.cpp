#include <string.h>

#include "redis_functions.h"
#include "zset.h"

// ptr = pointer to a struct's member
// T = type of the enclosing struct
// member = id for the field in T that ptr points to
#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

struct HKey {
    HNode node;
    std::string name = NULL;
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
    return memcmp(znode->key, &hkey->name, znode->key_len) == 0;
}

static ZNode* new_znode(double &score, std::string &key) {
    ZNode *znode = (ZNode*)malloc(sizeof(ZNode) + key.size());
    avl_init(&znode->avl_node);
    znode->h_node.next = NULL;
    znode->h_node.hcode = str_hash((uint8_t*)key.data(), key.size());

    znode->score = score;
    znode->key_len = key.size();
    memcpy(znode->key, key.data(), key.size());
    return znode;
}

void zdelete(ZSet *zset, ZNode *znode) {
    // Delete from the hashmap
    hm_delete(&zset->hmap, &znode->h_node, hcmp);

    // Delete from the avl tree
    zset->avl_root = avl_del(&znode->avl_node);
    free(znode);
}

void zinsert(ZSet *zset, double &score, std::string &key) {
    // Handle existing key
    ZNode *znode = new_znode(score, key);
    if (hm_lookup(&zset->hmap, &znode->h_node, hcmp)) {
        // Delete and insert again
        zdelete(zset, znode);
    }

    // Insert in the hashmap
    hm_insert(&zset->hmap, &znode->h_node);

    // Insert in the avl tree
    AVLNode **from = &zset->avl_root;
    AVLNode *curr = NULL;
    while (*from) {
        curr = *from;
        double val = container_of(curr, ZNode, avl_node)->score;
        from = val > score ? &curr->left : &curr->right;
    }

    *from = &znode->avl_node;
    zset->avl_root = avl_fix(&znode->avl_node);
}

ZNode* zlookup(ZSet* zset, std::string& key) {
    if (!zset->avl_root) {
        return NULL;
    }

    HKey hkey;
    hkey.len = key.size();
    hkey.name = key;
    hkey.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    // Do a hashmap lookup
    HNode *hnode = hm_lookup(&zset->hmap, &hkey.node, hcmp);
    return hnode ? container_of(hnode, ZNode, h_node) : NULL;
}
