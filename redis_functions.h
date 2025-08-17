#ifndef REDIS_FUNCTIONS_H
#define REDIS_FUNCTIONS_H

#include "data_structures/hashmap.h"
#include "data_structures/heap.h"
#include "server.h"

enum SIDE {
    LLIST_SIDE_LEFT = 0,
    LLIST_SIDE_RIGHT = 1
};

// Keyspace functions
void do_get(Conn *conn, std::vector<dstr*> &cmd);
void do_set(Conn *conn, std::vector<dstr*> &cmd);
void do_del(Conn *conn, std::vector<dstr*> &cmd);
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
void do_expire(Conn *conn, std::vector<dstr*> &cmd);
void do_ttl(Conn *conn, std::vector<HeapNode> &heap, std::string &key, uint32_t curr_ms);
void do_persist(Conn *conn, std::string &key);

// Hashmap functions
void do_hset(Conn *conn, std::vector<dstr*> &cmd);
void do_hget(Conn *conn, std::vector<dstr*> &cmd);
void do_hgetall(Conn *conn, std::vector<dstr*> &cmd);
void do_hdel(Conn *conn, std::vector<dstr*> &cmd);

// Linked list functions
void do_push(Conn *conn, std::vector<dstr*> &cmd, uint8_t side);
void do_pop(Conn *conn, std::vector<dstr*> &cmd, uint8_t side);
void do_lrange(Conn *conn, std::vector<dstr*> &cmd);

// Hashset functions
void do_sadd(Conn *conn, std::vector<dstr*> &cmd);
void do_srem(Conn *conn, std::vector<dstr*> &cmd);
void do_smembers(Conn *conn, std::vector<dstr*> &cmd);
void do_scard(Conn *conn, std::vector<dstr*> &cmd);

// Bitmap functions
void do_setbit(Conn *conn, std::vector<dstr*> &cmd);
void do_getbit(Conn *conn, std::vector<dstr*> &cmd);
void do_bitcount(Conn *conn, std::vector<dstr*> &cmd);

// HyperLogLog functions
void do_pfadd(Conn *conn, std::vector<dstr*> &cmd);
void do_pfcount(Conn *conn, std::vector<dstr*> &cmd);
void do_pfmerge(Conn *conn, std::vector<dstr*> &cmd);

#endif
