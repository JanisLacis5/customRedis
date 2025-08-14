#include <string.h>

#include "redis_functions.h"

#include <assert.h>

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

static bool validate_hmnode(Conn *conn, HNode *hm_node, uint32_t type) {
    if (!hm_node) {
        out_err(conn, "key does not exist");
        return false;
    }
    if (hm_node->type != type) {
        out_err(conn, "key already exists in database but is not the correct type");
        return false;
    }
    return true;
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
        HNode *hm_node = new_node(key, T_STR);
        hm_node->val = value;
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
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *hnode = hm_lookup(&global_data.db, &tmp);
    if (!hnode) {
        return out_null(conn);
    }

    set_ttl(hnode, ttl_ms);
    out_null(conn);
}

void do_ttl(Conn *conn, std::vector<HeapNode> &heap, std::string &key, uint32_t curr_ms) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *hnode = hm_lookup(&global_data.db, &tmp);
    if (!hnode) {
        return out_null(conn);
    }

    uint32_t ttl = (heap[hnode->heap_idx].val - curr_ms) / 1000;
    out_int(conn, ttl);
}

void do_persist(Conn *conn, std::string &key) {
    HNode tmp;
    tmp.key = key;
    tmp.hcode = str_hash((uint8_t*)key.data(), key.size());

    HNode *hnode = hm_lookup(&global_data.db, &tmp);
    if (!hnode) {
        return out_null(conn);
    }

    rem_ttl(hnode);
    out_null(conn);
}

void do_hset(Conn *conn, std::vector<std::string> &cmd) {
    // Find the hmap node
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        hm_node = new_node(cmd[1], T_HSET);
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
        HNode *set_node = new_node(cmd[2], T_STR);
        set_node->val = cmd[3];
        hm_insert(&hm_node->hmap, set_node);
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

void do_push(Conn *conn, std::vector<std::string> &cmd, uint8_t side) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        hm_node = new_node(cmd[1], T_LIST);
        hm_insert(&global_data.db, hm_node);
    }
    if (hm_node->type != T_LIST) {
        return out_err(conn, "node with the provided key exists and is not of type LIST");
    }

    DListNode *new_node = new DListNode();
    new_node->val = cmd[2];

    if (!hm_node->list.size) {
        hm_node->list.head = new_node;
        hm_node->list.tail = new_node;
    }
    else if (side == LLIST_SIDE_LEFT) {
        dlist_insert_before(hm_node->list.head, new_node);
        hm_node->list.head = new_node;
    }
    else if (side == LLIST_SIDE_RIGHT) {
        dlist_insert_after(hm_node->list.tail, new_node);
        hm_node->list.tail = new_node;
    }
    else {
        return out_err(conn, "internal error (do_push() side != 0 or 1)");
    }

    return out_int(conn, ++hm_node->list.size);
}

void do_pop(Conn *conn, std::vector<std::string> &cmd, uint8_t side) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_err(conn, "key does not exist in the database");
    }
    if (hm_node && hm_node->type != T_LIST) {
        return out_err(conn, "node with the provided key exists and is not of type LIST");
    }

    uint32_t size = cmd.size() > 2 ? stoi(cmd[2]) : 1;
    size = std::min(size, hm_node->list.size);
    if (size <= 0) {
        return out_err(conn, "value must be positive");
    }

    out_arr(conn, size);
    while (size--) {
        DListNode *node = side == LLIST_SIDE_LEFT ? hm_node->list.head : hm_node->list.tail;
        out_str(conn, node->val.data(), node->val.size());

        if (side == LLIST_SIDE_LEFT) {
            if (node->next) {
                node->next->prev = NULL;
            }
            hm_node->list.head = node->next;
        }
        else if (side == LLIST_SIDE_RIGHT) {
            if (node->prev) {
                node->prev->next = NULL;
            }
            hm_node->list.tail = node->prev;
        }
        else {
            out_err(conn, "internal error (do_pop() side not in [0 or 1])");
        }

        hm_node->list.size--;
        free(node);
    }
}

void do_lrange(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_err(conn, "key does not exist in the database");
    }
    if (hm_node->type != T_LIST) {
        return out_err(conn, "node with the provided key exists and is not of type LIST");
    }

    int32_t start = std::stoi(cmd[2]);
    int32_t end = std::stoi(cmd[3]);
    if (start < 0) {
        start = hm_node->list.size + start;
    }
    if (end < 0) {
        end = hm_node->list.size + end;
    }
    if (start >= hm_node->list.size) {
        return out_err(conn, "index out of range");
    }

    uint32_t l = start;
    uint32_t r = hm_node->list.size - start - 1;

    DListNode *begin = NULL;
    if (l < r) {
        begin = hm_node->list.tail;
        while (r--) {
            begin = begin->prev;
        }
    }
    else {
        begin = hm_node->list.head;
        while (l--) {
            begin = begin->next;
        }
    }

    uint32_t size = std::max(0, end - start + 1);
    size = std::min(size, hm_node->list.size);
    out_arr(conn, size);

    while (size--) {
        out_str(conn, begin->val.data(), begin->val.size());
        begin = begin->next;
    }
}

void do_sadd(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        hm_node = new_node(cmd[1], T_SET);
        hm_insert(&global_data.db, hm_node);
    }
    if (hm_node->type != T_SET) {
        return out_err(conn, "key already exists in the database and is not of type SET");
    }

    // Add a hnode without value - key will be the value
    HNode *in_node = new_node(cmd[2], T_STR);
    hm_insert(&hm_node->set, in_node);
}

void do_srem(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_err(conn, "node with the provided key does not exist");
    }
    if (hm_node->type != T_SET) {
        return out_err(conn, "key is not of type SET");
    }

    HNode key;
    key.key = cmd[2];
    key.hcode = str_hash((uint8_t*)cmd[2].data(), cmd[2].size());
    HNode *deleted = hm_delete(&hm_node->set, &key);
    return out_int(conn, (deleted ? 1 : 0));
}

void do_smembers(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_err(conn, "node with the provided key does not exist");
    }
    if (hm_node->type != T_SET) {
        return out_err(conn, "key is not of type SET");
    }

    std::vector<std::string> keys;
    hm_keys(&hm_node->set, keys);
    out_arr(conn, keys.size());
    for (std::string key: keys) {
        out_str(conn, key.data(), key.size());
    }
}

void do_scard(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        return out_err(conn, "node with the provided key does not exist");
    }
    if (hm_node->type != T_SET) {
        return out_err(conn, "key is not of type SET\n");
    }

    size_t size = hm_size(&hm_node->set);
    out_int(conn, size);
}


void do_setbit(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!hm_node) {
        hm_node = new_node(cmd[1], T_BITMAP);
        hm_insert(&global_data.db, hm_node);
    }
    if (hm_node->type != T_BITMAP) {
        return out_err(conn, "key already exists in database but is not of type BITMAP");
    }

    // Get the bit index
    int64_t bit_idx = stoll(cmd[2]);
    if (bit_idx < 0 || bit_idx > UINT32_MAX) {
        return out_err(conn, "index must be in range [0, 2^32-1]");
    }
    if (cmd[3] != "0" && cmd[3] != "1") {
        return out_err(conn, "bit has to be 0 or 1");
    }
    int64_t byte_idx = bit_idx / 8;
    bit_idx = 7 - (bit_idx % 8);


    // Extend the bitmap if necessary
    size_t bitmap_size = hm_node->bitmap.size();
    if (byte_idx >= bitmap_size) {
        hm_node->bitmap.resize(byte_idx + 1, '\0');
    }

    // Return the previous bit
    uint8_t byte = hm_node->bitmap[byte_idx];
    uint8_t prev = (byte >> bit_idx) & 1;
    out_int(conn, prev);

    // Set the bit
    if (cmd[3] == "1") {
        byte |= (1u << bit_idx);
    }
    else {
        byte &= ~(1u << bit_idx);
    }
    hm_node->bitmap[byte_idx] = byte;
}

void do_getbit(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!validate_hmnode(conn, hm_node, T_BITMAP)) {
        return;
    }

    int64_t bit_idx = stoll(cmd[2]);
    if (bit_idx < 0 || bit_idx / 8 >= hm_node->bitmap.size()) {
        return out_err(conn, "index outside of range");
    }
    uint32_t byte_idx = bit_idx / 8;
    bit_idx = 7 - (bit_idx % 8);

    uint8_t byte = hm_node->bitmap[byte_idx];
    uint32_t bit = (byte >> bit_idx) & 1;
    out_int(conn, bit);
}

void do_bitcount(Conn *conn, std::vector<std::string> &cmd) {
    HNode tmp;
    tmp.key = cmd[1];
    tmp.hcode = str_hash((uint8_t*)cmd[1].data(), cmd[1].size());

    HNode *hm_node = hm_lookup(&global_data.db, &tmp);
    if (!validate_hmnode(conn, hm_node, T_BITMAP)) {
        return;
    }

    int64_t start = cmd.size() > 2 ? stoll(cmd[2]) : 0;
    int64_t end = cmd.size() > 2 ? stoll(cmd[3]) : -1;
    bool is_byte_mode = cmd.size() == 5 && cmd[4] == "BYTE";

    size_t ret = 0;
    if (is_byte_mode) {
        if (start < 0) {
            start += hm_node->bitmap.size();
        }
        if (end < 0) {
            end += hm_node->bitmap.size();
        }
        end = std::min(end, (int64_t)hm_node->bitmap.size() - 1);

        for (int64_t byte_idx = start; byte_idx <= end; byte_idx++) {
            uint8_t byte = hm_node->bitmap[byte_idx];
            while (byte) {
                ret += (byte & 1);
                byte >>= 1;
            }
        }
    }
    else {
        size_t bit_count = hm_node->bitmap.size() * 8;
        if (start < 0) {
            start += bit_count;
        }
        if (end < 0) {
            end += bit_count;
        }
        end = std::min(end, (int64_t)bit_count - 1);

        for (int64_t bit_pos = start; bit_pos <= end; bit_pos++) {
            int64_t byte_idx = bit_pos / 8;
            int64_t bit_idx = 7 - (bit_pos % 8);
            ret += (hm_node->bitmap[byte_idx] >> bit_idx) & 1;
        }
    }
    out_int(conn, ret);
}

void do_pfadd(Conn *conn, std::vector<std::string> &cmd) {}

void do_pfcount(Conn *conn, std::vector<std::string> &cmd) {}

void do_pfmerge(Conn *conn, std::vector<std::string> &cmd) {}
