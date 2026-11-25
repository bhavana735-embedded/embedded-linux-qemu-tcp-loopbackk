#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int server_fd, client_fd;
    char buffer[1024] = {0};

    // Structure to store server address information
    struct sockaddr_in addr;

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to port 5000
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    // Listen for incoming client connections
    listen(server_fd, 1);

    printf("Waiting for client...\n");

    // Accept client connection
    client_fd = accept(server_fd, NULL, NULL);

    // Receive data from client
    recv(client_fd, buffer, sizeof(buffer), 0);

    printf("Received: %s\n", buffer);

    // Send the same data back to client (loopback)
    send(client_fd, buffer, strlen(buffer) + 1, 0);

    // Close sockets
    close(client_fd);
    close(server_fd);

    return 0;
}

