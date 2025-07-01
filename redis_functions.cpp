#include "buffer_funcs.h"
#include "hashmap.h"
#include "server.h"
#include "zset.h"

#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

// Response status codes
enum ResponseStatusCodes {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

enum Tags {
    TAG_INT = 0,
    TAG_STR = 1,
    TAG_ARR = 2,
    TAG_NULL = 3,
    TAG_ERROR = 4,
    TAG_DOUBLE = 5
};

enum EntryTypes {
    T_STR = 0,
    T_ZSET = 1
};

struct Entry {
    HNode node;
    std::string key;
    uint8_t type;

    // One of the variants
    std::string value;
    ZSet zset;
};

struct Lookup {
    HNode node;
    std::string key;
};

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

// FNV hash
static uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

static bool hm_keys_cb(HNode *node, std::vector<std::string> &keys) {
    std::string key = container_of(node, Entry, node)->value;
    keys.push_back(key);
    return true;
}

static Entry* new_entry(std::string &key, uint8_t type) {
    Entry *entry = new Entry();
    entry->key = key;
    entry->type = type;
    entry->node.hcode = str_hash((uint8_t*)key.data(), key.size());

    return entry;
}

static ZSet* find_zset(HMap *hmap, std::string &key) {
    // Find the zset
    Lookup l;
    l.key = key;
    l.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *zset_hnode = hm_lookup(hmap, &l.node, &entry_eq);
    if (!zset_hnode) {
        printf("ZSet with key %s does not exist\n", key);
        return NULL;
    }

    return &container_of(zset_hnode, Entry, zset)->zset;
}

void do_get(HMap *hmap, std::string &key, Conn *conn) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_lookup(hmap, &entry.node, entry_eq);

    if (!node) {
        conn->outgoing.resize(conn->outgoing.size() - 4); // remove the OK status code
        buf_append_u32(conn->outgoing, RES_NX);
        buf_append_u8(conn->outgoing, TAG_NULL);
    }
    else {
        std::string &val = container_of(node, Entry, node)->value;

        buf_append_u8(conn->outgoing, TAG_STR);
        buf_append_u32(conn->outgoing, (uint32_t)val.size());
        buf_append(conn->outgoing, (uint8_t*)val.data(), val.size());
    }
}

void do_set(HMap *hmap, Conn *conn, std::string &key, std::string &value) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());

    HNode *node = hm_lookup(hmap, &entry.node, entry_eq);
    if (node) {
        container_of(node, Entry, node) -> value = value;
    }
    else {
        Entry *e = new_entry(key, T_STR);
        e->value = value;
        hm_insert(hmap, &e->node);
    }
    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_del(HMap *hmap, Conn *conn, std::string &key) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_delete(hmap, &entry.node, entry_eq);
    if (!node) {
        delete container_of(node, Entry, node);
    }

    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_keys(HMap *hmap, Conn *conn) {
    buf_append_u8(conn->outgoing, TAG_ARR);

    std::vector<std::string> keys;
    hm_keys(hmap, &hm_keys_cb, keys);

    buf_append_u32(conn->outgoing, keys.size());
    for (std::string &key: keys) {
        buf_append_u8(conn->outgoing, TAG_STR);
        buf_append_u32(conn->outgoing, key.size());
        buf_append(conn->outgoing, (uint8_t*)key.data(), key.size());
    }
}

// Adds 1 if node was inserted, 0 if node was updated (this key existed in the set already)
void do_zadd(HMap *hmap, Conn *conn, std::string &global_key, double &score, std::string &z_key) {
    // Find the zset
    Lookup l;
    l.node.hcode = str_hash((uint8_t*)global_key.data(), global_key.size());
    l.key = global_key;
    HNode *node = hm_lookup(hmap, &l.node, &entry_eq);

    Entry *entry = NULL;
    if (!node) {
        entry = new_entry(global_key, T_ZSET);
    }
    else {
        entry = container_of(node, Entry, node);
        if (entry->type != T_ZSET) {
            buf_append_u8(conn->outgoing, TAG_ERROR);
            return;
        }
    }

    int inserted = zset_insert(&entry->zset, score, z_key);
    buf_append_u8(conn->outgoing, TAG_INT);
    buf_append_u32(conn->outgoing, inserted); // 1 if inserted else 0
}

// Adds SCORE if found, NULL if not found, ERROR if set does not exist
void do_zscore(HMap *hmap, Conn *conn, std::string &global_key, std::string &z_key) {
    ZSet *zset = find_zset(hmap, global_key);
    if (!zset) {
        buf_append_u8(conn->outgoing, TAG_ERROR);
        return;
    }

    ZNode *ret = zset_lookup(zset, z_key);
    if (!ret) {
        buf_append_u8(conn->outgoing, TAG_NULL);
        return;
    }

    buf_append_u8(conn->outgoing,TAG_DOUBLE);
    buf_append_double(conn->outgoing, ret->score);
}

// Adds 0 if the key or set was not found and not deleted, 1 if key was deleted
void do_zrem(HMap *hmap, Conn *conn, std::string &global_key, std::string &z_key) {
    // Find the zset
    ZSet *zset = find_zset(hmap, global_key);
    if (!zset) {
        buf_append_u8(conn->outgoing, TAG_INT);
        buf_append_u32(conn->outgoing, 0);
        return;
    }

    // Find and delete the node
    ZNode *znode = zset_lookup(zset, z_key);
    if (!znode) {
        buf_append_u8(conn->outgoing, TAG_INT);
        buf_append_u32(conn->outgoing, 0);
        return;
    }

    zset_delete(zset, znode);
    buf_append_u8(conn->outgoing, TAG_INT);
    buf_append_u32(conn->outgoing, 1);
}

/*
 *  HMap *hmap - global hashmap
 *  Conn *conn - connection between server and client
 *  std::string &global_key - key to zset in the global hashmap
 *  double score_lb - lower bound for score
 *  std::string &key_lb - lower bound for key
 *  uint32_t offset - how many qualifying tuples to skip before starting to return results
 *  uint32_t limit - the maximum number of pairs to return after applying the offset
 */
void do_zrangequery(
    HMap *hmap,
    Conn *conn,
    std::string &global_key,
    double score_lb,
    std::string &key_lb,
    uint32_t offset = 0,
    uint32_t limit = UINT32_MAX
) {

}




























