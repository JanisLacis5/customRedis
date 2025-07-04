#include <string.h>

#include "redis_functions.h"
#include "buffer_funcs.h"
#include "data_structures/hashmap.h"
#include "data_structures/heap.h"
#include "data_structures/zset.h"
#include "out_helpers.h"
#include "server.h"
#include "utils/common.h"
#include "utils/entry.h"

static bool hm_keys_cb(HNode *node, std::vector<std::string> &keys) {
    std::string key = container_of(node, Entry, node)->key;
    keys.push_back(key);
    return true;
}

static const ZSet empty;
static ZSet* find_zset(HMap *hmap, std::string &key) {
    // Find the zset
    Lookup l;
    l.key = key;
    l.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *zset_hnode = hm_lookup(hmap, &l.node, &entry_eq);
    if (!zset_hnode) {
        return (ZSet*)&empty;
    }

    Entry *entry = container_of(zset_hnode, Entry, node);
    return (entry->type < 2 && entry->type == T_ZSET) ? &entry->zset : NULL;
}

void do_get(std::string &key, Conn *conn) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_lookup(&global_data.db, &entry.node, entry_eq);

    if (!node) {
        out_not_found(conn);
    }
    else {
        std::string &val = container_of(node, Entry, node)->value;
        out_str(conn, val.data(), val.size());
    }
}

void do_set(Conn *conn, std::string &key, std::string &value) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());

    HNode *node = hm_lookup(&global_data.db, &entry.node, &entry_eq);
    if (node) {
        container_of(node, Entry, node)->value = value;
    }
    else {
        Entry *e = new_entry(key, T_STR);
        e->value = value;
        hm_insert(&global_data.db, &e->node);
    }
    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_del(Conn *conn, std::string &key) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_delete(&global_data.db, &entry.node, entry_eq);
    if (!node) {
        entry_del(container_of(node, Entry, node));
    }

    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_keys(Conn *conn) {
    std::vector<std::string> keys;
    hm_keys(&global_data.db, &hm_keys_cb, keys);

    out_arr(conn, keys.size());
    for (std::string &key: keys) {
        printf("%s\n", key.data());
        out_str(conn, key.data(), key.size());
    }
}

// Adds 1 if node was inserted, 0 if node was updated (this key existed in the set already)
void do_zadd(Conn *conn, std::string &global_key, double &score, std::string &z_key) {
    // Find the zset
    Lookup l;
    l.node.hcode = str_hash((uint8_t*)global_key.data(), global_key.size());
    l.key = global_key;
    HNode *node = hm_lookup(&global_data.db, &l.node, &entry_eq);

    Entry *entry = NULL;
    if (!node) {
        entry = new_entry(global_key, T_ZSET);
        hm_insert(&global_data.db, &entry->node);
    }
    else {
        entry = container_of(node, Entry, node);
        if (entry->type != T_ZSET) {
            return out_err(conn, "cant add zset to a non-zset node");
        }
    }

    int inserted = zset_insert(&entry->zset, score, z_key);
    out_int(conn, inserted);  // 1 if inserted else 0
}

// Adds SCORE if found, NULL if not found, ERROR if set does not exist
void do_zscore(Conn *conn, std::string &global_key, std::string &z_key) {
    ZSet *zset = find_zset(&global_data.db, global_key);
    ZNode *ret = zset_lookup(zset, z_key);
    if (!ret) {
        buf_append_u8(conn->outgoing, TAG_NULL);
        return;
    }
    out_double(conn, ret->score);
}

// Adds 0 if the key or set was not found and not deleted, 1 if key was deleted
void do_zrem(Conn *conn, std::string &global_key, std::string &z_key) {
    // Find the zset
    ZSet *zset = find_zset(&global_data.db, global_key);

    // Find and delete the node
    ZNode *znode = zset_lookup(zset, z_key);
    if (!znode) {
        out_int(conn, 0);
        return;
    }

    zset_delete(zset, znode);
    out_int(conn, 1);
}

/*
 *  Conn *conn - connection between server and client
 *  std::string &global_key - key to zset in the global hashmap
 *  double score_lb - lower bound for score
 *  std::string &key_lb - lower bound for key
 *  uint32_t offset - how many qualifying tuples to skip before starting to return results
 *  uint32_t limit - the maximum number of pairs to return after applying the offset
 */
void do_zrangequery(
    Conn *conn,
    std::string &global_key,
    double score_lb,
    std::string &key_lb,
    int32_t offset,
    uint32_t limit
) {
    ZSet *zset = find_zset(&global_data.db, global_key);

    // Find the first node that is in range
    ZNode *lb = zset_lower_bound(zset, score_lb, key_lb);
    if (!lb) {
        out_arr(conn, 0);
        return;
    }

    // Get the first node to return (offset)
    ZNode *znode = zset_offset(lb, offset);

    // Add array tag with unknown len to out buffer
    size_t size_pos = out_unknown_arr(conn);

    // Return the next LIMIT nodes
    uint32_t size = 0;
    while (znode && size < limit) {
        out_double(conn, znode->score);
        out_str(conn, znode->key, znode->key_len);
        znode = zset_offset(znode, 1);
        size += 2;
    }

    // Add size
    memcpy(&conn->outgoing[size_pos], &size, 4);
}

void do_expire(Conn *conn, std::string &key, uint32_t ttl_ms) {
    Lookup lkey;
    lkey.key = key;
    lkey.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node, &entry_eq);
    if (!entry_hnode) {
        printf("here\n");
        return out_null(conn);
    }

    Entry *entry = container_of(entry_hnode, Entry, node);
    ent_set_ttl(entry, ttl_ms);
    out_null(conn);
}

void do_ttl(Conn *conn, std::vector<HeapNode> &heap, std::string &key, uint32_t curr_ms) {
    Lookup lkey;
    lkey.key = key;
    lkey.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node, &entry_eq);
    if (!entry_hnode) {
        return out_null(conn);
    }

    Entry *entry = container_of(entry_hnode, Entry, node);
    uint32_t ttl = (heap[entry->heap_idx].val - curr_ms) / 1000;
    out_int(conn, ttl);
}

void do_persist(Conn *conn, std::string &key) {
    Lookup lkey;
    lkey.key = key;
    lkey.node.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node, &entry_eq);
    if (!entry_hnode) {
        return out_null(conn);
    }

    ent_rem_ttl(container_of(entry_hnode, Entry, node));
    out_null(conn);
}




























