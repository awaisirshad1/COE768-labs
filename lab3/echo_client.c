#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000    /* well-known port */
#define BUFLEN      100 /* buffer length */

int main(int argc, char **argv) {
    int n;
    int sd, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char *host, rbuf[BUFLEN], filename[256];
    int received_file_fd;

    switch(argc) {
        case 2:
            host = argv[1];
            port = SERVER_TCP_PORT;
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
            exit(1);
    }

    /* Create a stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (hp = gethostbyname(host)) {
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    } else if (inet_aton(host, (struct in_addr *)&server.sin_addr)) {
        fprintf(stderr, "Can't get server's address\n");
        exit(1);
    }

    /* Connecting to the server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't connect\n");
        exit(1);
    }

    // Get the filename from the user
    printf("Enter the filename to fetch: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        perror("Error reading filename");
        close(sd);
        return 1;
    }
    // Remove newline character from filename if present
    filename[strcspn(filename, "\n")] = '\0';

    // Send the filename to the server
    send(sd, filename, sizeof(filename), 0);


    // Open the file to save received data
    received_file_fd = open(filename, O_WRONLY | O_TRUNC, 0644);
    if (received_file_fd < 0) {
        perror("Failed to open file for writing");
        close(sd);
        return 1;
    }

    printf("Waiting for file data from server...\n");
    while (1) {
        // Read server response
        n = read(sd, rbuf, BUFLEN - 1);
        if (n > 0) {
            if (rbuf[0] == 1) { // Check if it's file data
                write(received_file_fd, rbuf + 1, n - 1); // Write to file, skipping first byte
            } else if (rbuf[0] == 0) { // Check for error message
                rbuf[n] = '\0'; // Null-terminate the error message
                printf("Error from server: %s\n", rbuf + 1); // Print the error message
                break;
            }
        } else if (n == 0) {
            // Server closed the connection
            printf("Server closed the connection.\n");
            break; // Exit the loop
        } else {
            perror("Error reading from server"); // Handle read error
            break; // Exit on error
        }
    }

    close(received_file_fd);
    close(sd);
    return 0;
}