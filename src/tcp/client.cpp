#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string>

const size_t MAX_MESSAGE_LEN = 4096;

static int read_all(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rec_len = read(fd, buf, n);
        if (rec_len <= 0) {
            return -1;
        }
        n -= rec_len;
        buf += rec_len;
    }
    return 0;
}

static int write_all(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t sent_len = write(fd, buf, n);
        if (sent_len <= 0) {
            return -1;
        }
        n -= sent_len;
        buf += sent_len;
    }
    return 0;
}

int handle_read(int fd) {
    // Receive a message
    std::vector<uint8_t> server_mes;
    server_mes.resize(4);
    errno = 0;
    int err = read_all(fd, server_mes.data(), 4);
    if (err) {
        printf("[client]: Error reading the message len\n");
        return -1;
    }
    uint32_t mes_len = 0;
    memcpy(&mes_len, server_mes.data(), 4);
    server_mes.resize(4 + mes_len);
    err = read_all(fd, &server_mes[4], mes_len);
    if (err) {
        printf("[client]: Error reading the message\n");
        return -1;
    }

    printf("Server says len:%d, message: %.*s\n", mes_len, (mes_len > 100 ? 100 : mes_len), &server_mes[4]);
    return 0;
}

int handle_write(int fd, const uint8_t *message, size_t mes_len) {
    // Sending custom messages
    std::vector<uint8_t> wbuf;
    wbuf.resize(4);
    memcpy(wbuf.data(), &mes_len, 4);

    wbuf.resize(4 + mes_len);
    memcpy(&wbuf[4], message, mes_len);
    int err = write_all(fd, wbuf.data(), wbuf.size());
    if (err) {
        printf("[client]: Error sending a message\n");
        return -1;
    }
    return err;
}

int main() {
    // Create the socket
    printf("[client]: Creating client socket...\n");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("[client]: Socket creation failed\n");
        return 1;
    }
    printf("[client]: Socket created!\n");

    // connect to the socket
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(0);
    int conn = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (conn == -1) {
        printf("[client]: Connection failed\n");
        return 1;
    }
    printf("[client]: Connection successful!\n");

    const size_t k = 32 << 20;
    std::vector<std::string> queries = {
        "query1",
        "query2",
        std::string(k, 'j'),
        "query4"
    };

    for (size_t i = 0; i < queries.size(); i++) {
        handle_write(fd, (uint8_t*)queries[i].data(), queries[i].size());
    }
    for (size_t i = 0; i < queries.size(); i++) {
        handle_read(fd);
    }
}
