#ifndef SERVER_H
#define SERVER_H

#include "dlist.h"
#include "utils/entry.h"


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

void ent_rem_ttl(Entry *entry);
void ent_set_ttl(Entry *entry, uint64_t ttl);

#endif
