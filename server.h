#ifndef SERVER_H
#define SERVER_H

struct DListNode {
    DListNode *prev = NULL;
    DListNode *next = NULL;
};

inline void dlist_init(DListNode *node) {
    node->prev = node;
    node->next = node;
}

inline bool dlist_empty(DListNode *node) {
    return node->next == node;
}

inline void dlist_deatach(DListNode *node) {
    DListNode *next = node->next;
    DListNode *prev = node->prev;

    if (prev) {
        prev->next = next;
    }
    if (next) {
        next->prev = prev;
    }
}

inline void dlist_insert_before(DListNode *target, DListNode *node) {
    DListNode *prev = target->prev;
    target->prev = node;
    node->next = target;
    prev->next = node;
    node->prev = prev;
}


struct Conn {
    // fd returned by poll() is non-negative
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // data for the app to process
    std::vector<uint8_t> outgoing; // responses

    DListNode timeout;
    uint64_t last_active_ms = 0;
};

#endif
