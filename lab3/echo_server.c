/* A simple file server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000    /* well-known port */
#define BUFLEN      256          /* buffer length */
#define MAX_FILENAME 256         /* maximum filename length */

/* Function prototypes */
void reaper(int sig);
void send_file(int client_socket, const char *filename);

int main(int argc, char **argv) {
    int sd, new_sd, client_len, port;
    struct sockaddr_in server, client;
    char filename[MAX_FILENAME];

    switch(argc) {
    case 1:
        port = SERVER_TCP_PORT;
        break;
    case 2:
        port = atoi(argv[1]);
        break;
    default:
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    /* Create a stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    /* Bind an address to the socket */
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't bind name to socket\n");
        exit(1);
    }

    /* Queue up to 5 connect requests */
    listen(sd, 5);

    (void) signal(SIGCHLD, reaper);

    while (1) {
        client_len = sizeof(client);
        new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
        if (new_sd < 0) {
            fprintf(stderr, "Can't accept client\n");
            continue; // Continue to accept other clients
        }

        // Read the filename from the client
        int n = read(new_sd, filename, sizeof(filename) - 1);
        if (n > 0) {
            filename[n] = '\0'; // Null-terminate the filename
            printf("Received request for file: %s\n", filename);
            send_file(new_sd, filename); // Send the requested file
        } else {
            perror("Error reading filename");
        }

        close(new_sd);
    }
    close(sd);
    return 0;
}

/* Function to send a file to the client */
void send_file(int client_socket, const char *filename) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        // File not found, send an error message
        char error_msg[BUFLEN];
        snprintf(error_msg, sizeof(error_msg), "Error: File not found.\n");
        write(client_socket, error_msg, strlen(error_msg));
        return;
    }

    char buffer[BUFLEN];
    int bytes_read;
   
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        // Send the first byte to indicate file data
        buffer[0] = 1; // Indicate that this is file data
        write(client_socket, buffer, bytes_read);
    }

    close(file_fd);
}

/* Reaper function to clean up terminated child processes */
void reaper(int sig) {
    int status;
    while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}