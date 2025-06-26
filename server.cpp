#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <vector>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <string>
#include <map>

#include "buffer_funcs.h"
#include "hashmap.h"

#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

const size_t MAX_MESSAGE_LEN = 32 << 20;
struct {
    HMap db;
} kv_store;

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

// Response status codes
enum ResponseStatusCodes {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

enum Tags {
    TAG_INT = 0,
    TAG_STR = 1,
    TAG_ARR = 2,
    TAG_NULL = 3,
    TAG_ERROR = 4,
    TAG_DOUBLE = 5
};

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

// FNV hash
static uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

void error(int fd, const char *mes) {
    close(fd);
    printf("[server]: %s\n", mes);
}

static void fd_set_non_blocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void do_get(std::string &key, Conn *conn) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_lookup(&kv_store.db, &entry.node, entry_eq);

    if (!node) {
        conn->outgoing.resize(conn->outgoing.size() - 4); // remove the OK status code
        buf_append_u32(conn->outgoing, RES_NX);
        buf_append_u8(conn->outgoing, TAG_NULL);
    }
    else {
        std::string &val = container_of(node, Entry, node)->value;

        buf_append_u8(conn->outgoing, TAG_STR);
        buf_append_u32(conn->outgoing, (uint32_t)val.size());
        buf_append(conn->outgoing, (uint8_t*)val.data(), val.size());
    }
}

void do_set(Conn *conn, std::string &key, std::string &value) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());

    HNode *node = hm_lookup(&kv_store.db, &entry.node, entry_eq);
    if (node) {
        container_of(node, Entry, node) -> value = value;
    }
    else {
        Entry *e = new Entry();
        e->key = entry.key;
        e->value = value;
        e->node.hcode = entry.node.hcode;
        hm_insert(&kv_store.db, &e->node);
    }
    buf_append_u8(conn->outgoing, TAG_NULL);
}

void do_del(Conn *conn, std::string &key) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_delete(&kv_store.db, &entry.node, entry_eq);
    if (!node) {
        delete container_of(node, Entry, node);
    }

    buf_append_u8(conn->outgoing, TAG_NULL);
}

bool hm_keys_cb(HNode *node, std::vector<std::string> &keys) {
    std::string key = container_of(node, Entry, node)->value;
    keys.push_back(key);
    return true;
}

void do_keys(Conn *conn) {
    buf_append_u8(conn->outgoing, TAG_ARR);

    std::vector<std::string> keys;
    hm_keys(&kv_store.db, &hm_keys_cb, keys);

    buf_append_u32(conn->outgoing, keys.size());
    for (std::string &key: keys) {
        buf_append_u8(conn->outgoing, TAG_STR);
        buf_append_u32(conn->outgoing, key.size());
        buf_append(conn->outgoing, (uint8_t*)key.data(), key.size());
    }
}

size_t parse_cmd(Conn *conn, std::vector<std::string> &cmd) {
    // Read the first tag (arr) and len
    uint8_t first_tag = 0;
    uint32_t nstr = 0;
    memcpy(&first_tag, &conn->incoming[0], 1);
    memcpy(&nstr, &conn->incoming[1], 4);

    // Read the token count
    const uint8_t *req = &conn->incoming[5];

    // Parse the request
    while (cmd.size() < nstr) {
        // Read the current token's size and tag (str)
        uint8_t tag;
        uint32_t t_len = 0;
        memcpy(&tag, req++, 1);
        memcpy(&t_len, req, 4);
        req += 4;

        // Read the token
        std::string token;
        token.resize(t_len);
        memcpy(token.data(), req, t_len);
        req += t_len;
        cmd.push_back(token);
    }

    return nstr;
}

void out_buffer(Conn *conn, std::vector<std::string> &cmd, const uint32_t &nstr) {
    if (nstr == 2 && cmd[0] == "get") {
        do_get(cmd[1], conn);
    }
    else if (nstr == 3 && cmd[0] == "set") {
        do_set(conn, cmd[1], cmd[2]);
    }
    else if (nstr == 2 && cmd[0] == "del") {
        do_del(conn, cmd[1]);
    }
    else if (nstr == 1 && cmd[0] == "keys") {
        do_keys(conn);
    }
    else {
        // TODO: add error message
        buf_append_u32(conn->outgoing, RES_ERR);
        buf_append_u8(conn->outgoing, TAG_NULL);
    }
}

void before_res_build(std::vector<uint8_t> &out, uint32_t &header) {
    // Add the first tags that will always be there
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, 2);
    buf_append_u8(out, TAG_INT);
    buf_append_u32(out, RES_OK);

    // Reserve size for the total message len
    header = out.size();
    buf_append_u32(out, 0);
}

void after_res_build(std::vector<uint8_t> &out, uint32_t &header) {
    // Add the header
    size_t mes_len = out.size() - header - 4;
    if (mes_len > MAX_MESSAGE_LEN) {
        printf("[server]: Message too long\n");
        return;
    }
    memcpy(&out[header], &mes_len, 4);
}

static bool try_one_req(Conn *conn) {
    if (conn->incoming.size() < 4) {
        // we need to read - we do not even know the message size
        return false;
    }

    // Read the header
    size_t total_len = 0;
    memcpy(&total_len, conn->incoming.data(), 4);

    // Parse the query
    std::vector<std::string> cmd;
    size_t nstr = parse_cmd(conn, cmd);

    // Log the query
    printf("[server]: Token from the client: ");
    for (std::string token: cmd) {
        printf("%s ", token.data());
    }
    printf("\n");

    // Add the first tags that will always be there
    uint32_t header_pos = 0;
    before_res_build(conn->outgoing, header_pos);

    // Create the output buffer. Each build starts with the status code
    out_buffer(conn, cmd, nstr);

    // Add the total length and clean up the incoming buffer
    after_res_build(conn->outgoing, header_pos);
    buf_consume(conn->incoming, 4 + total_len);

    return true;
}

static Conn* handle_accept(int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
        return NULL;
    }

    fd_set_non_blocking(connfd);

    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

static void handle_read(Conn *conn) {
    uint8_t rbuf[64*1024];
    int rv = read(conn->fd, rbuf, sizeof(rbuf));
    if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    if (rv <= 0) {
        conn->want_close = true;
        return;
    }

    buf_append(conn->incoming, rbuf, (size_t)rv);
    while(try_one_req(conn)) {}

    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;
    }
}

static void handle_write(Conn *conn) {
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    if (rv <= 0) {
        conn->want_close = true;
        return;
    }
    buf_consume(conn->outgoing, rv);

    if (conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

int main() {
    // Create
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("Error creating the socket\n");
        return -1;
    }
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0);
    addr.sin_port = htons(8000);
    int err = bind(fd, (struct sockaddr*)&addr, (socklen_t)sizeof(addr));
    if (err) {
        error(fd, "[server]: Error binding the socket\n");
        return -1;
    }

    // Listen
    err = listen(fd, SOMAXCONN);
    if (err) {
        error(fd, "Error listening to the socket\n");
        return -1;
    }

    std::vector<Conn*> fd_to_conn;
    std::vector<struct pollfd> poll_args;
    while (true) {
        // Add the current listening socket as first
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (Conn* conn: fd_to_conn) {
            if (!conn) {
                continue;
            }

            // Create poll() args for the current connection
            struct pollfd pfd = {conn->fd, POLLIN, 0};
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }
            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            poll_args.push_back(pfd);
        }

        // Call poll()
        int ret_val = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (ret_val < 0) {
            if (errno == EINTR) {
                // Nothing was ready
                continue;
            }
            else {
                error(fd, "Error calling poll()");
                return -1;
            }
        }

        // Handle the listening socket
        if (poll_args[0].revents) {
            Conn *conn = handle_accept(fd);
            if (!conn) {
                continue;
            }

            // Add this connection to the map
            if (fd_to_conn.size() <= (size_t)conn->fd) {
                fd_to_conn.resize(conn->fd+1);
            }
            fd_to_conn[conn->fd] = conn;
        }

        // Process the rest of the sockets (first on is the listening one so we start at i = 1)
        for (size_t i = 1; i < poll_args.size(); i++) {
            uint8_t ready = poll_args[i].revents;
            Conn *conn = fd_to_conn[poll_args[i].fd];
            if (ready & POLLIN) {
                handle_read(conn);
            }
            if (ready & POLLOUT) {
                handle_write(conn);
            }

            if ((ready & POLLERR) && conn->want_close) {
                close(conn->fd);
                fd_to_conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }
}