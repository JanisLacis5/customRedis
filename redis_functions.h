#ifndef REDIS_FUNCTIONS_H
#define REDIS_FUNCTIONS_H

#include "data_structures/hashmap.h"
#include "data_structures/heap.h"
#include "server.h"

// Hashmap functions
void do_get(std::string &key, Conn *conn);
void do_set(Conn *conn, std::string &key, std::string &value);
void do_del(Conn *conn, std::string &key);
void do_keys(Conn *conn);

// Sorted set functions
void do_zadd(Conn *conn, std::string &key, double &score, std::string &name);
void do_zscore(Conn *conn, std::string &global_key, std::string &z_key);
void do_zrem(Conn *conn, std::string &key, std::string &name);
void do_zrangequery(
    Conn *conn,
    std::string &global_key,
    double score_lb,
    std::string &key_lb,
    int32_t offset = 0,
    uint32_t limit = UINT32_MAX
);
void do_expire(Conn *conn, std::string &key, uint32_t ttl_ms);
void do_ttl(Conn *conn, std::vector<HeapNode> &heap, std::string &key, uint32_t curr_ms);
void do_persist(Conn *conn, std::string &key);

#endif
