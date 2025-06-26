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
enum {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

struct Response {
    uint32_t status_code = RES_OK;
    std::vector<uint8_t> data;
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

void do_get(std::string &key, Response &res) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_lookup(&kv_store.db, &entry.node, entry_eq);

    if (!node) {
        res.status_code = RES_NX;
    }
    else {
        std::string &val = container_of(node, Entry, node)->value;
        res.data.assign(val.begin(), val.end());
    }
}

void do_set(std::string &key, std::string &value) {
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
}

void do_del(std::string &key) {
    Entry entry;
    entry.key = key;
    entry.node.hcode = str_hash((uint8_t*)entry.key.data(), entry.key.size());
    HNode *node = hm_delete(&kv_store.db, &entry.node, entry_eq);
    if (!node) {
        delete container_of(node, Entry, node);
    }
}

static bool try_one_req(Conn *conn) {
    if (conn->incoming.size() < 4) {
        // we need to read - we do not even know the message size
        return false;
    }

    // Read the len of the whole message
    uint32_t total_len = 0;
    memcpy(&total_len, conn->incoming.data(), 4);
    if (total_len > MAX_MESSAGE_LEN) {
        conn->want_close = true;
        return false;
    }
    if (4 + total_len > conn->incoming.size()) {
        return false; // want read
    }

    // Read the token count
    const uint8_t *req = &conn->incoming[4];
    uint32_t nstr = 0;
    memcpy(&nstr, req, 4);
    req += 4;

    // Parse the request
    std::vector<std::string> cmd;
    while (cmd.size() < nstr) {
        // Read the current token's size
        uint32_t t_len = 0;
        memcpy(&t_len, req, 4);
        req += 4;

        // Read the token
        std::string token;
        token.resize(t_len);
        memcpy(token.data(), req, t_len);
        req += t_len;
        cmd.push_back(token);
    }

    // Log the query
    printf("[server]: Token from the client: ");
    for (std::string token: cmd) {
        printf("%s ", token.data());
    }
    printf("\n");

    // Create a response
    Response res;
    if (nstr == 2 && cmd[0] == "get") {
        do_get(cmd[1], res);
    }
    else if (nstr == 3 && cmd[0] == "set") {
        do_set(cmd[1], cmd[2]);
    }
    else if (nstr == 2 && cmd[0] == "del") {
        do_del(cmd[1]);
    }
    else {
        res.status_code = RES_ERR;
    }

    // Add the response to a buffer (total size | status_code | data)
    uint32_t res_len = 4 + (uint32_t)res.data.size();
    buf_append(conn->outgoing, (uint8_t*)&res_len, 4);
    buf_append(conn->outgoing, (uint8_t*)&res.status_code, 4);
    buf_append(conn->outgoing, res.data.data(), res.data.size());
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