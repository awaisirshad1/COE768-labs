/* time_client.c - main */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>

                                                                           

#include <arpa/inet.h>


#define BUFSIZE 100
#define FILENAME_MAX 100

struct pdu {
    char type;      // PDU type: C, D, F, E
    char data[BUFSIZE]; // Data field with a maximum size of 100 bytes
};

int main(int argc, char **argv) {
    char *host = "localhost";
    int port = 3000;
    struct sockaddr_in server_addr; // Server address
    int sockfd;                     // Socket file descriptor
    struct pdu spdu;               // PDU structure
    char filename[FILENAME_MAX];    // Buffer for filename
    socklen_t addr_len = sizeof(server_addr);
    int received_file;
    // Parse command line arguments
    switch (argc) {
        case 1:
            break;
        case 2:
            host = argv[1];
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "usage: UDPfileClient [host [port]]\n");
            exit(1);
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Can't crint received_file_fd;eate socket");
        exit(1);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    // Loop to allow multiple downloads
    while (1) {
        printf("Enter filename to download (or 'exit' to quit): ");
        scanf("%s", filename);
        if (strcmp(filename, "exit") == 0) {
            break;
        }
        printf("the filename you entered is %s", filename);

        // Prepare FILENAME PDU
        spdu.type = 'C';
        strncpy(spdu.data, filename, sizeof(spdu.data) - 1);
        spdu.data[sizeof(spdu.data) - 1] = '\0'; // Null terminate

        // Send FILENAME PDU
        sendto(sockfd, &spdu, sizeof(spdu), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        FILE *file = fopen(filename, "w");
        if(file == NULL){
            perror("Error opening/creating the file");
        }
        //create a file with filename:
        //received_file = open(filename, O_WRONLY | O_TRUNC, 0644);
        // file error handling 
        // if (received_file < 0) {
        //     perror("Failed to open file for writing");
        //     close(sockfd);
        //     return 1;
        // }
        // Receive file data
        while (1) {
            recvfrom(sockfd, &spdu, sizeof(spdu), 0, (struct sockaddr *)&server_addr, &addr_len);
            if (spdu.type == 'E') {
                printf("Error: %s\n", spdu.data);
                break;
            } else if (spdu.type == 'F') {
                printf("Received final batch of file data.\n");
                //write to file and close
                fprintf(file, spdu.data, sizeof(spdu.data));
                fclose(file);
                //write(received_file, spdu.data, 100);
                // Save the final batch or process accordingly
                break;
            } else if (spdu.type == 'D') {
                printf("Received data: %s\n", spdu.data);
                // write to file
                fprintf(file, spdu.data, 100, sizeof(char), );
                //write(received_file, spdu.data, sizeof(spdu.data));
                //write(received_file, '\0', 1);
                // Save data to file or process accordingly
            }
        }
    }

    close(sockfd);
    return 0;
}
