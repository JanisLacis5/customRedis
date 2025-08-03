#ifndef SERVER_H
#define SERVER_H

#include "dlist.h"
#include "threadpool.h"
#include "data_structures/hashmap.h"
#include "data_structures/heap.h"

struct Conn {
    // fd returned by poll() is non-negative
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // data for the app to process
    std::vector<uint8_t> outgoing; // responses

    DListNode idle_timeout;
    DListNode read_timeout;
    DListNode write_timeout;
    uint64_t last_active_ms = 0;
    uint64_t last_read_ms = 0;
    uint64_t last_write_ms = 0;
};

struct GlobalData {
    HMap db;
    DListNode idle_list;
    DListNode read_list;
    DListNode write_list;
    ThreadPool threadpool;
    std::vector<Conn*> fd_to_conn;
    std::vector<HeapNode> ttl_heap;
};

extern GlobalData global_data;
uint64_t get_curr_ms();
void set_ttl(HNode *node, uint64_t ttl);
void rem_ttl(HNode *node);

#endif
