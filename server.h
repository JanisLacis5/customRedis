#ifndef SERVER_H
#define SERVER_H

struct Entry {
    HNode node;
    std::string key;
    std::string value;
};

struct Conn {
    // fd returned by poll() is non-negative
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // data for the app to process
    std::vector<uint8_t> outgoing; // responses
};

#endif
