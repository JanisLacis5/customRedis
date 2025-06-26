#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string>

#include "buffer_funcs.h"

const size_t MAX_MESSAGE_LEN = 4096;

enum {
    TAG_INT = 0,
    TAG_STR = 1,
    TAG_ARR = 2,
    TAG_NULL = 3,
    TAG_ERROR = 4,
    TAG_DOUBLE = 5
};

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
    if (err < 0) {
        printf("[client]: Error reading the message len\n");
        return -1;
    }
    // Get the total size of the message (status code + data)
    uint32_t mes_len = 0;
    memcpy(&mes_len, &server_mes[0], 4);
    server_mes.resize(4 + mes_len);

    // Read the rest of the message (status code + data)
    err = read_all(fd, &server_mes[4], mes_len);

    // Get the status
    uint32_t status_code = 0;
    memcpy(&status_code, &server_mes[4], 4);

    const uint8_t *data = &server_mes[8];
    printf("Status code = %d, data: %s\n", status_code, data);
    return 0;
}

int handle_write(int fd, std::vector<std::string> &query) {
    std::vector<uint8_t> buf;

    // Add tag and len (always array because query can contain many arguments)
    buf_append_u8(buf, (uint8_t)TAG_ARR);
    buf_append_u32(buf, (uint32_t)query.size());

    // Add each argument
    for (const std::string &token: query) {
        uint32_t len = token.size();
        // Add tag and len for the current token
        buf_append_u8(buf, (uint8_t)TAG_STR);
        buf_append_u32(buf, len);

        // Add the value
        buf_append(buf, (uint8_t*)token.data(), len);
    }

    if (buf.size() > MAX_MESSAGE_LEN) {
        printf("[client]: Message too long\n");
        return -1;
    }
    return write_all(fd, buf.data(), buf.size());
}

int main(int argc, char **argv) {
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

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
        cmd.push_back(argv[i]);
    }
    printf("\n");
    int err = handle_write(fd, cmd);
    if (err) {
        close(fd);
        printf("[client]: Error sending the query\n");
        return -1;
    }
    err = handle_read(fd);
    if (err) {
        close(fd);
        printf("[client]: Error reading the response\n");
        return -1;
    }
}
