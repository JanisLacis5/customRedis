#ifndef REDIS_FUNCTIONS_H
#define REDIS_FUNCTIONS_H

#include "hashmap.h"
#include "server.h"
#include "zset.h"

struct Lookup {
    HNode node;
    std::string key;
};

// Hashmap functions
void do_get(HMap *hmap, std::string &key, Conn *conn);
void do_set(HMap *hmap, Conn *conn, std::string &key, std::string &value);
void do_del(HMap *hmap, Conn *conn, std::string &key);
void do_keys(HMap *hmap, Conn *conn);

// Sorted set functions
void do_zadd(HMap *hmap, Conn *conn, std::string &key, double &score, std::string &name);
void do_zscore(HMap *hmap, Conn *conn, std::string &global_key, std::string &z_key);
void do_zrem(HMap *hmap, Conn *conn, std::string &key, std::string &name);
void do_zrangequery(
    HMap *hmap,
    Conn *conn,
    std::string &global_key,
    double score_lb,
    std::string &key_lb,
    int32_t offset = 0,
    uint32_t limit = UINT32_MAX
);

#endif
