#include <stdbool.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>


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
            return 1;
        }

        char client_mes[100];
        char server_mes[100] = "Hello from server!";

        // Receive message from the client
        ssize_t client_mes_len = read(connfd, &client_mes, sizeof(client_mes));
        if (client_mes_len < 0) {
            close(connfd);
            printf("[server]: Message reading failed\n");
            continue;
        }

        // Respond
        ssize_t server_mes_len = write(connfd, &server_mes, sizeof(server_mes));
        if (server_mes_len < 0) {
            close(connfd);
            printf("[server]: Sending message failed\n");
            continue;
        }
        close(connfd);
    }
}