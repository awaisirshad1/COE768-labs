/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

struct pdu {
    char type;      // PDU type: C, D, F, E
    char data[100]; // Data field with a maximum size of 100 bytes
};

int main(int argc, char *argv[]) {
    struct sockaddr_in client_addr; // The from address of a client
    struct sockaddr_in server_addr;  // Server address
    struct pdu request;              // Received request PDU
    struct pdu response;             // Response PDU
    int sockfd;                      // Server socket
    socklen_t addr_len = sizeof(client_addr);
    int file_fd;

    // Parse command line arguments
    int port = 3000;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Can't create socket");
        exit(1);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Can't bind to port");
        exit(1);
    }

    // Loop to handle requests
    while (1) {
        // Receive FILENAME PDU
        recvfrom(sockfd, &request, sizeof(request), 0, (struct sockaddr *)&client_addr, &addr_len);

        if (request.type == 'C') {
            // Open the requested file
            file_fd = open(request.data, O_RDONLY);
            if (file_fd < 0) {
                response.type = 'E';
                strncpy(response.data, "File not found", sizeof(response.data) - 1);
                sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_len);
                continue;
            }

            // Read and send file data in chunks
            ssize_t bytes_read;
            while ((bytes_read = read(file_fd, response.data, sizeof(response.data))) > 0) {
                response.type = 'D';
                sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_len);
            }

            // Send FINAL PDU
            response.type = 'F';
            sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_len);
            close(file_fd);
        }
    }

    close(sockfd);
    return 0;
}
