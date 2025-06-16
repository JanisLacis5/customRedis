#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

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

    // Sending custom messages
    while (true) {
        printf("Send a message to the server!\n");

        char client_mes[100];
        scanf("%s", &client_mes);
        if (strcmp(client_mes, "exit") == 0) {
            close(fd);
            printf("[client]: Connection disconnected!\n");
            return 0;
        }
        ssize_t client_mes_len = write(fd, client_mes, sizeof(client_mes));
        if (client_mes_len < 0) {
            close(fd);
            printf("[client]: Client message sending failed\n");
            return 1;
        }

        // Receive a message
        char server_mes[100];
        ssize_t server_mes_len = read(fd, server_mes, sizeof(server_mes));
        if (server_mes_len < 0) {
            close(fd);
            printf("[client]: Error reveiving a message from the server\n");
            return 1;
        }

        printf("Client sent: %s\n", client_mes);
        printf("Server answered: %s\n", server_mes);
    }
}