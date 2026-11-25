#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sockfd;
    char buffer[1024] = {0};

    // Structure to store server address information
    struct sockaddr_in server;

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);

    // // Connect to server at 10.0.2.15
    server.sin_addr.s_addr = inet_addr("10.0.2.15");

    // Establish connection with server
    connect(sockfd, (struct sockaddr *)&server, sizeof(server));

    // Send message to server
    send(sockfd, "Hello RN Embedded", 18, 0);

    // Receive echoed message from server
    recv(sockfd, buffer, sizeof(buffer), 0);

    // Display received data
    printf("Loopback Received: %s\n", buffer);

    // Close socket
    close(sockfd);

    return 0;
}
