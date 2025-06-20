#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t MAX_MESSAGE_LEN = 4096;

static int read_all(int fd, char *buf, size_t n) {
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

static int write_all(int fd, char *buf, size_t n) {
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

int query(int fd, char *message) {
    // Sending custom messages
    char wbuf[4 + MAX_MESSAGE_LEN];
    uint32_t mes_len = strlen(message);
    memcpy(wbuf, &mes_len, 4);
    memcpy(&wbuf[4], message, mes_len);
    int err = write_all(fd, wbuf, 4 + mes_len);
    if (err) {
        printf("[client]: Error sending a message\n");
        return -1;
    }

    // Receive a message
    char server_mes[4 + MAX_MESSAGE_LEN + 1];
    errno = 0;
    err = read_all(fd, server_mes, 4);
    if (err) {
        printf("[client]: Error reading the message len\n");
        return -1;
    }
    memcpy(&mes_len, server_mes, 4);
    err = read_all(fd, &server_mes[4], mes_len);
    if (err) {
        printf("[client]: Error reading the message\n");
        return -1;
    }

    printf("Server says: %s\n", &server_mes[4]);
    return 0;
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

    int err = query(fd, "Hi from client n1");
    if (err) {
        close(fd);
        return 0;
    }
    err = query(fd, "Hi from client n2");
    if (err) {
        close(fd);
        return 0;
    }
    err = query(fd, "Hi from client n3");
    if (err) {
        close(fd);
        return 0;
    }
}