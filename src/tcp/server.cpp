#include <errno.h>
#include <bits/stdint-uintn.h>
#include <sys/socket.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <vector>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

const size_t MAX_MESSAGE_LEN = 32 << 20;

struct Conn {
    // fd returned by poll() is non-negative
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // data for the app to process
    std::vector<uint8_t> outgoing; // responses
};

void error(int fd, const char *mes) {
    close(fd);
    printf("[server]: %s\n", mes);
}

static void fd_set_non_blocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data+len);
}

static void buf_consume(std::vector<uint8_t> &buf, size_t len) {
    buf.erase(buf.begin(), buf.begin() + len);
}

static bool try_one_req(struct Conn *conn) {
    if (conn->incoming.size() < 4) {
        // we need to read - we do not even know the message size
        return false;
    }

    uint32_t mes_len = 0;
    memcpy(&mes_len, conn->incoming.data(), 4);
    if (mes_len > MAX_MESSAGE_LEN) {
        conn->want_close = true;
        return false;
    }
    if (mes_len + 4 > conn->incoming.size()) {
        return false; // need to read the whole message
    }

    const uint8_t *req = &conn->incoming[4];
    printf("[server]: Client says len: %d, messsage: %.100s\n", mes_len, req);
    buf_append(conn->outgoing, (const uint8_t*)&mes_len, 4);
    buf_append(conn->outgoing, req, mes_len);
    buf_consume(conn->incoming, 4 + mes_len);
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
            fd_to_conn[fd+1] = conn;
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