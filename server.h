#ifndef SERVER_H
#define SERVER_H

#include "dlist.h"
#include "zset.h"

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

struct Entry {
    HNode node;
    std::string key;
    uint32_t type = 2;

    // One of the variants
    std::string value;
    ZSet zset;

    // For ttl heap
    size_t heap_idx = -1;
};

void entry_del(Entry *entry);

#endif
