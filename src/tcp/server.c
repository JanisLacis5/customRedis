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

int main() {
    // Create the socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("[server]: Socket creation failed\n");
        return 1;
    }
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    printf("[server]: Server socket created!\n");

    // Bind the socket to 0.0.0.0:8000
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(0);

    int bind_ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (bind_ret == -1) {
        close(fd);
        printf("[server]: Binding failed\n");
        return 1;
    }
    printf("[server]: Binding successful!\n");

    // Listen to the socket
    int listen_ret = listen(fd, SOMAXCONN);
    if (listen_ret == -1) {
        close(fd);
        printf("[server]: listening failed\n");
        return 1;
    }

    // Accept connections
    printf("[server]: Accepting connections...\n");
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd == -1) {
            printf("[server]: Accepting failed\n");
            continue;
        }

        while (true) {
            // Receive message from the client
            char message[4 + MAX_MESSAGE_LEN];
            errno = 0;
            int err = read_all(connfd, message, 4);
            if (err) {
                close(connfd);
                printf(errno == 0 ? "[server]: EOF\\n" : "[server]: Error in reading the message length\n");
                return err;
            }

            uint32_t mes_len = 0;
            memcpy(&mes_len, message, 4);
            err = read_all(connfd, &message[4], mes_len);
            if (err) {
                close(connfd);
                printf("[server]: Error in reading the message\n");
                return err;
            }
            printf("[server]: Client says: %s\n", &message[4]);

            // Respond
            char reply[] = "Hello from the server!";
            char wbuf[4 + sizeof(reply)];
            mes_len = strlen(reply);
            memcpy(wbuf, &mes_len, 4);
            memcpy(&wbuf[4], reply, mes_len);
            err = write_all(connfd, wbuf, 4 + mes_len);
            if (err) {
                close(connfd);
                printf("[server]: Error sending the response\n");
                return err;
            }
        }
        close(connfd);
    }
}