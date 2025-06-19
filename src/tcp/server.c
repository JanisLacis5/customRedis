#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t MAX_MESSAGE_LEN = 4096;

static int read_all(int fd, char *buf, ssize_t n) {
    while (n > 0) {
        ssize_t rec_len = read(fd, buf, n);
        if (rec_len <= 0) {
            return -1;
        }
        n -= rec_len;
    }
    return 0;
}

void write_all() {

}

int main() {
    // Create the socket
    printf("[server]: Creating server socket...\n");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("[server]: Socket creation failed\n");
        return 1;
    }
    printf("[server]: Server socket created!\n");

    // Bind the socket to 0.0.0.0:8000
    printf("[server]: Binding server socket...\n");
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
            close(fd);
            printf("[server]: Accepting failed\n");
            continue;
        }

        char client_mes[100];
        char server_mes[100] = "Hello from server!";

        while (true) {
            // Receive message from the client
            char message[4 + MAX_MESSAGE_LEN];
            int err = read_all(connfd, message, 4);

            errno = 0;
            if (err) {
                // ASK FOR THIS
                printf(errno == 0 ? "[server]: EOF\\n" : "[server]: Error in reading the message length\n");
                return -1;
            }

            uint32_t mes_len = 0;
            memcpy(&mes_len, message, 4);
            printf("Received the messsage len: %d\n", mes_len);
            err = read_all(connfd, &message[4], mes_len);
            if (err) {
                printf("[server]: Error in reading the message\n");
                return -1;
            }
            printf("[server]: Client says: %s", &message[4]);

            // Respond
        }
        close(connfd);
    }
}