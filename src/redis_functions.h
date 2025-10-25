#ifndef REDIS_FUNCTIONS_H
#define REDIS_FUNCTIONS_H

#include "data_structures/heap.h"
#include "server.h"

enum SIDE {
    LLIST_SIDE_LEFT = 0,
    LLIST_SIDE_RIGHT = 1
};

enum RETURN_VALS {
    SUCCESS = 0,
    NOT_FOUND = 1,
    INCORRECT_TYPE = 2,
    SIZE_ERR = 3,
    OUT_OF_RANGE = 4,
    INTERNAL_ERR = 5
};

// Keyspace functions
uint8_t do_get(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_set(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_del(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_keys(Conn* conn);

// Sorted set functions
uint8_t do_zadd(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_zscore(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_zrem(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_zrangequery(Conn* conn, std::vector<dstr*>& cmd);

// TLL functions
uint8_t do_expire(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_ttl(Conn* conn, std::vector<dstr*>& cmd, std::vector<HeapNode>& heap, uint32_t curr_ms);
uint8_t do_persist(Conn* conn, std::vector<dstr*>& cmd);

// Hashmap functions
uint8_t do_hset(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_hget(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_hgetall(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_hdel(Conn* conn, std::vector<dstr*>& cmd);

// Linked list functions
uint8_t do_push(Conn* conn, std::vector<dstr*>& cmd, uint8_t side);
uint8_t do_pop(Conn* conn, std::vector<dstr*>& cmd, uint8_t side);
uint8_t do_lrange(Conn* conn, std::vector<dstr*>& cmd);

// Hashset functions
uint8_t do_sadd(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_srem(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_smembers(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_scard(Conn* conn, std::vector<dstr*>& cmd);

// Bitmap functions
uint8_t do_setbit(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_getbit(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_bitcount(Conn* conn, std::vector<dstr*>& cmd);

// HyperLogLog functions
uint8_t do_pfadd(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_pfcount(Conn* conn, std::vector<dstr*>& cmd);
uint8_t do_pfmerge(Conn* conn, std::vector<dstr*>& cmd);

#endif
