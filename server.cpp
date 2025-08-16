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
#include <time.h>

#include "server.h"
#include "data_structures/dlist.h"
#include "data_structures/zset.h"
#include "buffer_funcs.h"
#include "data_structures/hashmap.h"
#include "redis_functions.h"
#include "data_structures/avl_tree.h"
#include "data_structures/heap.h"
#include "out_helpers.h"
#include "utils/common.h"
#include "threadpool.h"

GlobalData global_data;
const size_t MAX_MESSAGE_LEN = 32 << 20;
const uint32_t MAX_TTL_TASKS = 200;
const uint64_t IDLE_TIMEOUT_MS = 10 * 1000;
const uint64_t READ_TIMEOUT_MS = 10 * 000;
const uint64_t WRITE_TIMEOUT_MS = 5 * 1000;

static void error(int fd, const char *mes) {
    close(fd);
    printf("[server]: %s\n", mes);
}

static void fd_set_non_blocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

uint64_t get_curr_ms() {
    struct timespec tv = {0, 0};
    int err = clock_gettime(CLOCK_MONOTONIC, &tv);
    if (err) {
        printf("[server]: error in get_curr_ms\n");
        return 0;
    }
    return tv.tv_sec * 1000 + tv.tv_nsec / 1000 / 1000;
}

static void close_conn(Conn *conn) {
    close(conn->fd);
    global_data.fd_to_conn[conn->fd] = NULL;
    dlist_deatach(&conn->idle_timeout);
    dlist_deatach(&conn->read_timeout);
    dlist_deatach(&conn->write_timeout);
    delete conn;
}

static bool hnode_same(HNode *node, HNode *key) {
    return node == key;
}

static void process_timers() {
    uint64_t curr_ms = get_curr_ms();

    // Idle timeout
    while (!dlist_empty(&global_data.idle_list)) {
        DListNode *curr = global_data.idle_list.next;
        Conn *conn = container_of(curr, Conn, idle_timeout);
        uint64_t timeout_ms = conn->last_active_ms + IDLE_TIMEOUT_MS;
        if (timeout_ms >= curr_ms) {
            break;
        }

        printf("[server]: Closing conn %d because of idle timeout\n", conn->fd);
        close_conn(conn);
    }

    // Read timeout
    while (!dlist_empty(&global_data.read_list)) {
        DListNode *curr = global_data.read_list.next;
        Conn *conn = container_of(curr, Conn, read_timeout);
        uint64_t timeout_ms = conn->last_read_ms + READ_TIMEOUT_MS;
        if (timeout_ms >= curr_ms) {
            break;
        }

        printf("[server]: Closing conn %d because of read timeout\n", conn->fd);
        close_conn(conn);
    }

    // Write timeout
    while (!dlist_empty(&global_data.write_list)) {
        DListNode *curr = global_data.write_list.next;
        Conn *conn = container_of(curr, Conn, write_timeout);
        uint64_t timeout_ms = conn->last_write_ms + WRITE_TIMEOUT_MS;
        if (timeout_ms >= curr_ms) {
            break;
        }

        printf("[server]: Closing conn %d because of write timeout\n", conn->fd);
        close_conn(conn);
    }

    // Entry timeouts
    size_t curr_iterations = 0;
    while (!global_data.ttl_heap.empty() && global_data.ttl_heap[0].val < curr_ms) {
        HNode *hnode = container_of(global_data.ttl_heap[0].pos_ref, HNode, heap_idx);
        rem_ttl(hnode);
        HNode *deleted = hm_delete(&global_data.db, hnode);

        if (curr_iterations++ >= MAX_TTL_TASKS) {
            break;
        }
    }
}

void set_ttl(HNode *node, uint64_t ttl) {
    HeapNode hn;
    hn.val = get_curr_ms() + ttl;
    hn.pos_ref = &node->heap_idx;

    global_data.ttl_heap.push_back(hn);
    heap_fix(global_data.ttl_heap, global_data.ttl_heap.size() - 1);
}

void rem_ttl(HNode *node) {
    if (node->heap_idx < 0 || node->heap_idx >= global_data.ttl_heap.size()) {
        return;
    }

    // Remove from the heap
    global_data.ttl_heap[node->heap_idx] = global_data.ttl_heap.back();
    global_data.ttl_heap.pop_back();

    if (node->heap_idx < global_data.ttl_heap.size()) {
        heap_fix(global_data.ttl_heap, node->heap_idx);
    }
}

static uint64_t next_timer_ms() {
    if (dlist_empty(&global_data.idle_list)) {
        return (int64_t)IDLE_TIMEOUT_MS;
    }
    uint64_t curr = get_curr_ms();

    // Get the smallest timer from the sockets
    Conn *conn = container_of(global_data.idle_list.next, Conn, idle_timeout);
    uint64_t conn_timeout = conn->last_active_ms + IDLE_TIMEOUT_MS;

    // Check if there is a smaller entry timeout
    if (!global_data.ttl_heap.empty()) {
        conn_timeout = std::min(conn_timeout, global_data.ttl_heap[0].val);
    }

    uint64_t poll_timout = conn_timeout - curr;
    return std::max((uint64_t)0, poll_timout);
}

static size_t parse_cmd(uint8_t *buf, std::vector<dstr*> &cmd) {
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
        dstr *token = dstr_init(t_len);
        dstr_append(&token, (char*)buf, t_len);
        buf += t_len;
        cmd.push_back(token);
    }

    return nstr;
}

static void out_buffer(Conn *conn, std::vector<dstr*> &cmd) {
    // GLOBAL DATABASE
    if (cmd.size() == 2 && !strcmp(cmd[0]->buf, "get")) {
        do_get(conn, cmd);
    }
    else if (cmd.size() == 3 && !strcmp(cmd[0]->buf, "set")) {
        do_set(conn, cmd);
    }
    else if (cmd.size() == 2 && !strcmp(cmd[0]->buf, "del")) {
        do_del(conn, cmd);
    }
    else if (cmd.size() == 1 && !strcmp(cmd[0]->buf, "keys")) {
        do_keys(conn);
    }

    // // HASHMAP
    // else if (cmd.size() >= 4 && (cmd.size() & 1) == 0 && cmd[0] == "hset") {
    //     do_hset(conn, cmd);
    // }
    // else if (cmd.size() == 3 && cmd[0] == "hget") {
    //     do_hget(conn, cmd);
    // }
    // else if (cmd.size() == 2 && cmd[0] == "hgetall") {
    //     do_hgetall(conn, cmd);
    // }
    // else if (cmd.size() >= 3 && cmd[0] == "hdel") {
    //     do_hdel(conn, cmd);
    // }
    //
    // // TIME TO LIVE
    // else if (cmd.size() == 3 && cmd[0] == "expire") {
    //     uint32_t ttl_ms = std::stoi(cmd[2]) * 1000;
    //     do_expire(conn, cmd[1], ttl_ms);
    // }
    // else if (cmd.size() == 2 && cmd[0] == "ttl") {
    //     do_ttl(conn, global_data.ttl_heap, cmd[1], get_curr_ms());
    // }
    // else if (cmd.size() == 2 && cmd[0] == "persist") {
    //     do_persist(conn, cmd[1]);
    // }
    //
    // // SORTED SET
    // else if (cmd.size() == 4 && cmd[0] == "zadd") {
    //     double score = std::stod(cmd[2]);
    //     do_zadd(conn, cmd[1], score, cmd[3]);
    // }
    // else if (cmd.size() == 3 && cmd[0] == "zscore") {
    //     do_zscore(conn, cmd[1], cmd[2]);
    // }
    // else if (cmd.size() == 3 && cmd[0] == "zrem") {
    //     do_zrem(conn, cmd[1], cmd[2]);
    // }
    // else if (cmd[0] == "zquery") {
    //     double score = std::stod(cmd[2]);
    //     int32_t offset = std::stoi(cmd[4]);
    //     uint32_t limit = std::stoi(cmd[5]);
    //     do_zrangequery(
    //         conn,
    //         cmd[1],
    //         score,
    //         cmd[3],
    //         offset,
    //         limit
    //     );
    // }
    //
    // // LINKED LIST
    // else if (cmd[0] == "lpush" && cmd.size() == 3) {
    //     do_push(conn, cmd, LLIST_SIDE_LEFT);
    // }
    // else if (cmd[0] == "rpush" && cmd.size() == 3) {
    //     do_push(conn, cmd, LLIST_SIDE_RIGHT);
    // }
    // else if (cmd[0] == "lpop" && cmd.size() >= 2) {
    //     do_pop(conn, cmd, LLIST_SIDE_LEFT);
    // }
    // else if (cmd[0] == "rpop" && cmd.size() >= 2) {
    //     do_pop(conn, cmd, LLIST_SIDE_RIGHT);
    // }
    // else if (cmd[0] == "lrange" && cmd.size() == 4) {
    //     do_lrange(conn, cmd);
    // }
    //
    // // HASHSET
    // else if (cmd[0] == "sadd") {
    //     do_sadd(conn, cmd);
    // }
    // else if (cmd[0] == "srem") {
    //     do_srem(conn, cmd);
    // }
    // else if (cmd[0] == "smembers") {
    //     do_smembers(conn, cmd);
    // }
    // else if (cmd[0] == "scard") {
    //     do_scard(conn, cmd);
    // }
    //
    // // BITMAP
    // else if (cmd[0] == "setbit") {
    //     do_setbit(conn, cmd);
    // }
    // else if (cmd[0] == "getbit") {
    //     do_getbit(conn, cmd);
    // }
    // else if (cmd[0] == "bitcount") {
    //     do_bitcount(conn, cmd);
    // }
    //
    // // HYPERLOGLOG
    // else if (cmd[0] == "pfadd") {
    //     do_pfadd(conn, cmd);
    // }
    // else if (cmd[0] == "pfcount") {
    //     do_pfcount(conn, cmd);
    // }
    // else if (cmd[0] == "pfmerge") {
    //     do_pfmerge(conn, cmd);
    // }
    else {
        out_err(conn, "unknown command");
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
    std::vector<dstr*> cmd;
    parse_cmd(&conn->incoming[4], cmd);

    // Log the query
    printf("[server]: Token from the client: ");
    for (dstr *token: cmd) {
        printf("%s ", token->buf);
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
    conn->last_active_ms = get_curr_ms();
    conn->last_read_ms = get_curr_ms();
    conn->last_write_ms = get_curr_ms();
    dlist_insert_before(&global_data.idle_list, &conn->idle_timeout);
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
        dlist_deatach(&conn->read_timeout);
        return;
    }

    buf_append(conn->incoming, rbuf, (size_t)rv);
    while(try_one_req(conn)) {}
    conn->last_read_ms = get_curr_ms();

    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;
        dlist_deatach(&conn->read_timeout);
        dlist_insert_before(&global_data.write_list, &conn->write_timeout);
        conn->last_write_ms = get_curr_ms();
    }
}

static void handle_write(Conn *conn) {
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    if (rv <= 0) {
        conn->want_close = true;
        dlist_deatach(&conn->write_timeout);
        return;
    }
    buf_consume(conn->outgoing, rv);
    conn->last_write_ms = get_curr_ms();

    if (conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
        dlist_deatach(&conn->write_timeout);
        dlist_insert_before(&global_data.read_list, &conn->read_timeout);
        conn->last_read_ms = get_curr_ms();
    }
}

int main() {
    // Initialize global data
    threadpool_init(&global_data.threadpool, 8);
    dlist_init(&global_data.idle_list);
    dlist_init(&global_data.read_list);
    dlist_init(&global_data.write_list);

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

    std::vector<struct pollfd> poll_args;
    while (true) {
        // Add the current listening socket as first
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (Conn* conn: global_data.fd_to_conn) {
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
        int ret_val = poll(poll_args.data(), (nfds_t)poll_args.size(), next_timer_ms());
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
            if (global_data.fd_to_conn.size() <= (size_t)conn->fd) {
                global_data.fd_to_conn.resize(conn->fd+1);
            }
            global_data.fd_to_conn[conn->fd] = conn;
        }

        // Process the rest of the sockets (first on is the listening one so we start at i = 1)
        for (size_t i = 1; i < poll_args.size(); i++) {
            uint8_t ready = poll_args[i].revents;
            if (ready == 0) {
                continue;
            }

            Conn *conn = global_data.fd_to_conn[poll_args[i].fd];
            conn->last_active_ms = get_curr_ms();
            dlist_deatach(&conn->idle_timeout);
            dlist_insert_before(&global_data.idle_list, &conn->idle_timeout);

            if (ready & POLLIN) {
                handle_read(conn);
            }
            if (ready & POLLOUT) {
                handle_write(conn);
            }
            if ((ready & POLLERR) && conn->want_close) {
                close_conn(conn);
            }

        }
        process_timers();
    }
}