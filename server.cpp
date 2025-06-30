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
#include "server.h"
#include "redis_functions.h"
#include "zset.h"

#define container_of(ptr, T, member) ((T *)( (char *)ptr - offsetof(T, member) ))

const size_t MAX_MESSAGE_LEN = 32 << 20;
struct {
    HMap db;
} kv_store;

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

static void error(int fd, const char *mes) {
    close(fd);
    printf("[server]: %s\n", mes);
}

static void fd_set_non_blocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static size_t parse_cmd(uint8_t *buf, std::vector<std::string> &cmd) {
    // Read the first tag (arr) and len
    uint8_t first_tag = 0;
    uint32_t nstr = 0;
    memcpy(&first_tag, buf++, 1);
    memcpy(&nstr, buf, 4);
    buf += 4;

    // Parse the request
    while (cmd.size() < nstr) {
        // Read the current token's size and tag (str)
        uint8_t tag;
        uint32_t t_len = 0;
        memcpy(&tag, buf++, 1);
        memcpy(&t_len, buf, 4);
        buf += 4;

        // Read the token
        std::string token;
        token.resize(t_len);
        memcpy(token.data(), buf, t_len);
        buf += t_len;
        cmd.push_back(token);
    }

    return nstr;
}

static void out_buffer(Conn *conn, std::vector<std::string> &cmd) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        do_get(&kv_store.db, cmd[1], conn);
    }
    else if (cmd.size() == 3 && cmd[0] == "set") {
        do_set(&kv_store.db, conn, cmd[1], cmd[2]);
    }
    else if (cmd.size() == 2 && cmd[0] == "del") {
        do_del(&kv_store.db, conn, cmd[1]);
    }
    else if (cmd.size() == 1 && cmd[0] == "keys") {
        do_keys(&kv_store.db, conn);
    }
    else if (cmd.size() == 4 && cmd[0] == "zadd") {
        double score = std::stod(cmd[2]);
        do_zadd(&kv_store.db, conn, cmd[1], score, cmd[3]);
    }
    else if (cmd.size() == 3 && cmd[0] == "zscore") {
        do_zscore(&kv_store.db, conn, cmd[1], cmd[2]);
    }
    else if (cmd.size() == 3 && cmd[0] == "zrem") {
        do_zrem(&kv_store.db, conn, cmd[1], cmd[2]);
    }
    else {
        // TODO: add error message
        buf_append_u32(conn->outgoing, RES_ERR);
        buf_append_u8(conn->outgoing, TAG_NULL);
    }
}

static void before_res_build(std::vector<uint8_t> &out, uint32_t &header) {
    // Reserve size for the total message len
    header = out.size();
    buf_append_u32(out, 0);

    // Add the first tags that will always be there
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, 2);
    buf_append_u8(out, TAG_INT);
    buf_append_u32(out, RES_OK);
}

static void after_res_build(std::vector<uint8_t> &out, uint32_t &header) {
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

    if (conn->incoming.size() < total_len + 4) {
        // need read
        return false;
    }

    // Parse the query
    std::vector<std::string> cmd;
    size_t nstr = parse_cmd(&conn->incoming[4], cmd);

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
    out_buffer(conn, cmd);

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