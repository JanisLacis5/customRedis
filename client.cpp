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
    // Sending custom messages
    uint32_t total_len = 4;
    for (std::string token: query) {
        // Add size for the token and it's size
        total_len += 4 + token.size();
    }
    if (total_len > MAX_MESSAGE_LEN) {
        printf("[client]: Message too long\n");
        return -1;
    }

    std::vector<uint8_t> wbuf(4 + total_len);
    memcpy(&wbuf[0], &total_len, 4);

    uint32_t q_len = query.size();
    memcpy(&wbuf[4], &q_len, 4);

    uint8_t *curr = &wbuf[8];
    for (std::string token: query) {
        uint32_t mes_len = token.size();
        memcpy(curr, &mes_len, 4);
        curr += 4;

        memcpy(curr, token.data(), mes_len);
        curr += mes_len;
    }

    return write_all(fd, wbuf.data(), wbuf.size());
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
