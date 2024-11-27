#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/signal.h>

// Define constants for quit message, server port, connection limits, and buffer sizes
#define QUIT "quit"
#define SERVER_PORT 10000
#define CONNECTION_REQUEST_LIMIT 5
#define BUFFER_LENGTH 100
#define NAME_SIZE 20
#define MAX_CONNECTIONS 200

// Define the PDU (Protocol Data Unit) structures for UDP packets and size-based PDUs
typedef struct {
    char messageType;            // Message type (e.g., R, T, L)
    char messageData[BUFFER_LENGTH]; // Data field for the UDP packet
} PDU_UDP;

typedef struct {
    char messageType;              // Message type (e.g., R, T, L)
    char messageData[BUFFER_LENGTH]; // Data field for the UDP packet
    int dataSize;                   // Size of the data in the packet
} PDU_WithSize;

// This structure tracks the list of registered files on the server
PDU_UDP registrationPDU;

// Structure to track files and their status
struct {
    int fileStatus;             // Status of the file (1 - registered, 0 - deregistered)
    char fileName[NAME_SIZE];   // Name of the file
} registeredFileTable[MAX_CONNECTIONS]; // Table to store registered files

// Global variables for user and socket information
char userName[NAME_SIZE];
int serverSocket, peerPort;
int tcpSocket, numberOfFileDescriptors;
fd_set readFileDescriptors, allFileDescriptors;

void registerContent(int, char * );
int searchForContent(int, char * , PDU_UDP * );
int downloadContentFromClient(char * , PDU_UDP * );
int handleServerDownload(int);
void deregisterContent(int, char * );
void listOnlineUsers(int);
void listLocalContent();
void exitProgram(int);
void signalHandler();
void childProcessReaper(int);
int currentIndex = 0;
char peerUserName[10];
int processId;

int main(int argc, char ** argv) {
    int serverPort = SERVER_PORT;
    int bytesRead;
    int addressLength = sizeof(struct sockaddr_in);
    struct hostent * hostEntry;
    struct sockaddr_in serverAddress;
    char commandCharacter, * host, peerName[NAME_SIZE];
    struct sigaction signalAction;
    char dataToSend[100];

    // Parse command-line arguments for host and port
    switch (argc) {
    case 2:
        host = argv[1];
        break;
    case 3:
        host = argv[1];
        serverPort = atoi(argv[2]);
        break;
    default:
        printf("Usage: %s host [port]\n", argv[0]);
        exit(1);
    }

    // Create a TCP socket for communication with the client server
    int tcpSocketDescriptor, newSocketDescriptor, clientAddressLength, portNumber, m;
    struct sockaddr_in clientAddress, clientAddress2;

    if ((tcpSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Unable to create a socket\n");
        exit(1);
    }

    char localIP[16];
    bzero((char * ) & clientAddress, sizeof(struct sockaddr_in));
    socklen_t clientAddressLength = sizeof(clientAddress);

    if (bind(tcpSocketDescriptor, (struct sockaddr * ) & clientAddress, sizeof(clientAddress)) == -1) {
        fprintf(stderr, "Unable to bind address to socket\n");
        exit(1);
    }

    // Start listening for incoming connection requests
    listen(tcpSocketDescriptor, CONNECTION_REQUEST_LIMIT);

    // Set up signal handler for child processes
    (void) signal(SIGCHLD, childProcessReaper);

    // Set up UDP communication with the server
    memset(&serverAddress, 0, addressLength);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (hostEntry = gethostbyname(host))
        memcpy(&serverAddress.sin_addr, hostEntry->h_addr, hostEntry->h_length);
    else if ((serverAddress.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        printf("Unable to get host entry\n");
        exit(1);
    }

    // Create a UDP socket for communication with the index server
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        printf("Unable to create socket\n");
        exit(1);
    }

    // Connect to the index server using the UDP socket
    if (connect(serverSocket, (struct sockaddr * ) & serverAddress, sizeof(serverAddress)) < 0) {
        printf("Unable to connect to the server\n");
        exit(1);
    }

    // Get the port number the client has opened and display the local IP address
    getsockname(tcpSocketDescriptor, (struct sockaddr * ) & clientAddress, &clientAddressLength);
    inet_ntop(AF_INET, &clientAddress.sin_addr, localIP, sizeof(localIP));
    int localPort = ntohs(clientAddress.sin_port);
    char * ipAddress = inet_ntoa(((struct sockaddr_in * ) & clientAddress)->sin_addr);
    char localAddress[45];
    char localPortString[25];
    sprintf(localAddress, "%s", localIP);
    sprintf(localPortString, "%u", localPort);

    // Prompt user to choose a user name
    printf("Choose a user name\n");
    bytesRead = read(0, peerUserName, 10);

    // Initialize the file descriptor sets and register sockets
    FD_ZERO(&allFileDescriptors);
    FD_SET(serverSocket, &allFileDescriptors);   // Listen to the index server
    FD_SET(0, &allFileDescriptors);               // Listen to user input
    FD_SET(tcpSocketDescriptor, &allFileDescriptors); // Listen to the TCP client server

    numberOfFileDescriptors = 1;
    for (bytesRead = 0; bytesRead < MAX_CONNECTIONS; bytesRead++)
        registeredFileTable[bytesRead].fileStatus = -1; // Initialize all file statuses to -1

    // Set up signal handler for SIGINT (Ctrl+C)
    signalAction.sa_handler = signalHandler;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = 0;
    sigaction(SIGINT, &signalAction, NULL);

    // Fork a child process for handling TCP server operations
    switch ((processId = fork())) {
    case 0: // Child process for handling TCP connections
        while (1) {
            clientAddressLength = sizeof(clientAddress2);
            newSocketDescriptor = accept(tcpSocketDescriptor, (struct sockaddr * ) & clientAddress2, &clientAddressLength);
            if (newSocketDescriptor < 0) {
                fprintf(stderr, "Unable to accept client connection\n");
                exit(1);
            }

            // Fork again to handle multiple clients
            switch (fork()) {
            case 0: // Child process to handle a specific client download request
                (void) close(tcpSocketDescriptor);
                exit(handleServerDownload(newSocketDescriptor));
            default: // Parent process to continue listening for new clients
                (void) close(newSocketDescriptor);
                break;
            case -1:
                fprintf(stderr, "Error creating a new process\n");
            }
        }
    default:
        // Main loop for handling user commands
        while (1) {
            printf("Enter command:\n");

            // Copy file descriptors for select
            memcpy(&readFileDescriptors, &allFileDescriptors, sizeof(readFileDescriptors));
            if (select(numberOfFileDescriptors, &readFileDescriptors, NULL, NULL, NULL) == -1) {
                printf("select error: %s\n", strerror(errno));
                exit(1);
            }

            // Check if input is from the user
            if (FD_ISSET(0, &readFileDescriptors)) {
                char command[40];
                bytesRead = read(0, command, 40);

                // Command help option
                if (command[0] == '?') {
                    printf("R - Register content; T - Deregister content; L - List local content\n");
                    printf("D - Download content; O - List online content; Q - Quit\n\n");
                    continue;
                }

                // Register content
                if (command[0] == 'R') {
                    registrationPDU.messageType = 'R';
                    printf("Enter content name:\n");
                    char contentName[20];
                    bytesRead = read(0, contentName, 20);
                    peerUserName[strcspn(peerUserName, "\n")] = 0;
                    contentName[strcspn(contentName, "\n")] = 0;

                    if (fopen(contentName, "r") == NULL) {
                        printf("File does not exist!\n");
                        continue;
                    }

                    // Send registration request with username, content name, and port number
                    strcpy(dataToSend, peerUserName);
                    strcat(dataToSend, ";");
                    strcat(dataToSend, contentName);
                    strcat(dataToSend, ";");
                    sprintf(dataToSend + strlen(dataToSend), "%d", localPort);

                    // Send registration PDU
                    registerContent(serverSocket, dataToSend);
                }
            }
        }
    }
}

// Register content on the server
void registerContent(int socket, char * data) {
    int sendResult;
    sendResult = send(socket, data, strlen(data), 0); // Send registration message
    if (sendResult < 0) {
        printf("Error sending registration message\n");
        exit(1);
    }
}

// Handle server download request
int handleServerDownload(int socket) {
    return 0; // Placeholder function, implement your logic
}

// Reap child processes after handling them
void childProcessReaper(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Exit the program
void exitProgram(int signal) {
    exit(0);
}

// Handle SIGINT signal (Ctrl+C)
void signalHandler() {
    printf("\nExiting program...\n");
    exit(0);
}
