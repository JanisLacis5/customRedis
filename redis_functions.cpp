#include <string.h>

#include "redis_functions.h"
#include "buffer_funcs.h"
#include "data_structures/hashmap.h"
#include "data_structures/heap.h"
#include "data_structures/zset.h"
#include "out_helpers.h"
#include "server.h"
#include "utils/common.h"

static const ZSet empty;
static ZSet* find_zset(HMap *hmap, std::string &key) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *node = hm_lookup(hmap, &tmp);
    if (!node) {
        return (ZSet*)&empty;
    }

    return node->type == T_ZSET ? node->zset : NULL;
}

void do_get(std::string &key, Conn *conn) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *node = hm_lookup(&global_data.db, &tmp);
    if (!node) {
        out_not_found(conn);
    }
    else {
        out_str(conn, node->val.data(), node->val.size());
    }
}


void do_set(Conn *conn, std::string &key, std::string &value) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *node = hm_lookup(&global_data.db, &tmp);
    if (node) {
        node->val = value;
    }
    else {
        HNode *hm_node = new HNode();
        hm_node->val = value;
        hm_node->key = key;
        hm_node->type = T_STR;
        hm_node->hcode = str_hash((uint8_t*)key.data(), key.size());
        hm_insert(&global_data.db, hm_node);
    }
    buf_append_u8(conn->outgoing, TAG_NULL);
}


void do_del(Conn *conn, std::string &key) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)tmp.key.data(), tmp.key.size());

    hm_delete(&global_data.db, &tmp);
    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_keys(Conn *conn) {
    std::vector<std::string> keys;
    hm_keys(&global_data.db, keys);

    out_arr(conn, keys.size());
    for (std::string &key: keys) {
        printf("%s\n", key.data());
        out_str(conn, key.data(), key.size());
    }
}

// Adds 1 if node was inserted, 0 if node was updated (this key existed in the set already)
void do_zadd(Conn *conn, std::string &keyspace_key, double &score, std::string &z_key) {
    // Find the zset
    HNode tmp;
    tmp.key = keyspace_key;
    tmp.hcode = str_hash((uint8_t*)keyspace_key.data(), keyspace_key.size());
    HNode *node = hm_lookup(&global_data.db, &tmp);

    if (!node) {
        node = new_node(keyspace_key, T_ZSET);
        hm_insert(&global_data.db, node);
    }

    if (node->type != T_ZSET) {
        return out_err(conn, "cant add zset to a non-zset node");
    }

    int inserted = zset_insert(node->zset, score, z_key);
    out_int(conn, inserted);  // 1 if inserted else 0
}

// Adds SCORE if found, NULL if not found, ERROR if set does not exist
void do_zscore(Conn *conn, std::string &keyspace_key, std::string &z_key) {
    ZSet *zset = find_zset(&global_data.db, keyspace_key);
    ZNode *ret = zset_lookup(zset, z_key);
    if (!ret) {
        buf_append_u8(conn->outgoing, TAG_NULL);
        return;
    }
    out_double(conn, ret->score);
}

// Adds 0 if the key or set was not found and not deleted, 1 if key was deleted
void do_zrem(Conn *conn, std::string &keyspace_key, std::string &z_key) {
    // Find the zset
    ZSet *zset = find_zset(&global_data.db, keyspace_key);

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
 *  std::string &keyspace_key - key to zset in the global hashmap
 *  double score_lb - lower bound for score
 *  std::string &key_lb - lower bound for key
 *  uint32_t offset - how many qualifying tuples to skip before starting to return results
 *  uint32_t limit - the maximum number of pairs to return after applying the offset
 */
void do_zrangequery(
    Conn *conn,
    std::string &keyspace_key,
    double score_lb,
    std::string &key_lb,
    int32_t offset,
    uint32_t limit
) {
    ZSet *zset = find_zset(&global_data.db, keyspace_key);

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

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node);
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

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node);
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

    HNode *entry_hnode = hm_lookup(&global_data.db, &lkey.node);
    if (!entry_hnode) {
        return out_null(conn);
    }

    ent_rem_ttl(container_of(entry_hnode, Entry, node));
    out_null(conn);
}

void do_hset(Conn *conn, std::vector<std::string> &cmd) {
    // Find the hmap node
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        hm_node = new HNode();
        hm_node->key = cmd[1];
        hm_node->type = T_HSET;
        hm_node->hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

        hm_insert(&global_data.db, hm_node);
    }

    // Find hmap entry
    if (hm_node->type != T_HSET) {
        return out_err(conn, "keyspace key is not of type hashmap");
    }

    // Find the key node in the entry hashmap
    HNode hm_tmp;
    hm_tmp.key = cmd[2];
    hm_tmp.hcode = str_hash((uint8_t*)cmd[2].data(), cmd[2].size());
    HNode *node = hm_lookup(&hm_node->hmap, &hm_tmp);

    if (node) {
        if (node->type != T_STR) {
            return out_err(conn, "[hset] node is not of type STR");
        }
        node->val = cmd[3];
    }
    else {
        HNode *new_node = new HNode();
        new_node->key = cmd[2];
        new_node->hcode = str_hash((uint8_t*)cmd[2].data(), cmd[2].size());
        new_node->type = T_STR;
        new_node->val = cmd[3];

        hm_insert(&hm_node->hmap, new_node);
    }
    out_null(conn);
}

void do_hget(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    // Find the hashmap in which the hget is being done
    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_null(conn);
    }

    if (hm_node->type != T_HSET) {
        return out_err(conn, "keyspace key is not of type hashmap");
    }

    // Find the key node in the hashmap
    HNode hm_tmp;
    hm_tmp.key = cmd[2];
    hm_tmp.hcode = str_hash((uint8_t*)cmd[2].data(), cmd[2].size());
    HNode *node = hm_lookup(&hm_node->hmap, &hm_tmp);

    if (node && node->type == T_STR) {
        return out_str(conn, node->val.data(), node->val.size());
    }
    out_null(conn);
}

void do_hdel(Conn *conn, std::vector<std::string> &cmd) {
    // Find the hmap node
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());
    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_null(conn);
    }

    // Find hmap entry
    if (hm_node->type != T_HSET) {
        return out_err(conn, "keyspace key is not of type hashmap");
    }

    // Find the key node in the entry hashmap
    HNode hm_tmp;
    hm_tmp.key = cmd[2];
    hm_tmp.hcode = str_hash((uint8_t*)cmd[2].data(), cmd[2].size());
    HNode *node = hm_delete(&hm_node->hmap, &hm_tmp);

    // Delete hmap entry if it's hmap is empty
    if (hm_size(&hm_node->hmap) == 0) {
        hm_delete(&global_data.db, hm_node);
    }
    out_null(conn);
}

void do_hgetall(Conn *conn, std::vector<std::string> &cmd) {
    // Find the hmap node
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_null(conn);
    }

    std::vector<std::string> keys;
    hm_keys(&hm_node->hmap, keys);

    out_arr(conn, keys.size());
    for (std::string &key: keys) {
        out_str(conn, key.data(), key.size());
    }
}
