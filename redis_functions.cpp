#include "buffer_funcs.h"
#include "hashmap.h"
#include "server.h"

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
        Entry *e = new Entry();
        e->key = entry.key;
        e->value = value;
        e->node.hcode = entry.node.hcode;
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