#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>


#define FILE_NAME_MAX 20
#define UPD_MAX_BUF 100
#define MAX_LEDGER_ENTRIES 200
#define NOT_FOUND "Content Not Found"


typedef struct ledger_entry 
{
    char content_host[FILE_NAME_MAX];
    char ip_addr[16];
    char port_num[7];
    short token;
    struct ledger_entry * next;
}   LEDGER_ENTRY;

typedef struct 
{
    char content_name[FILE_NAME_MAX]; // Content Name
    LEDGER_ENTRY * head;
}   LEDGER;
    LEDGER list[MAX_LEDGER_ENTRIES];

int max_index = 0;

// Pdu for udp communication
typedef struct 
{
    char type;
    char data[UPD_MAX_BUF];
}   PDU_UDP;
PDU_UDP tpdu;

// METHOD SIGNATURES
// find content by name
void search_content(int, char * , struct sockaddr_in * );
// register a peer as content server
void registration(int, char * , struct sockaddr_in * );
// de-register a peer
void deregistration(int, char * , struct sockaddr_in * );

struct sockaddr_in fsin;

// UDP Content Indexing Service
int main(int argc, char * argv[]) {
    struct sockaddr_in sin, * p_addr; // the from address of a client	
    LEDGER_ENTRY * p_entry;
    char * service = "10000"; // service content_name or port_num number	
    char content_name[FILE_NAME_MAX], content_host[FILE_NAME_MAX];
    int alen = sizeof(struct sockaddr_in); // from-address length		
    int s, n, i, len, p_sock; // socket descriptor and socket type    
    int pdulen = sizeof(PDU_UDP);
    struct hostent * hp;
    PDU_UDP udp_pdu;
    int j = 0;

    PDU_UDP spdu;

    for (n = 0; n < MAX_LEDGER_ENTRIES; n++)
        list[n].head = NULL;
    switch (argc) {
    case 1:
        break;
    case 2:
        service = argv[1];
        break;
    default:
        fprintf(stderr, "Incorrect Arguments \n Use the format: server [host] [port_num]\n");
    }

    memset( & sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    // Map service content_name to port_num number 
    sin.sin_port = htons((u_short) atoi(service));

    // Allocate socket 
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        fprintf(stderr, "can't creat socket\n");
        exit(1);
    }

    // Bind socket 
    if (bind(s, (struct sockaddr * ) & sin, sizeof(sin)) < 0)
        fprintf(stderr, "can't bind to %s port_num\n", service);
    // receive UDP connections
    while (1) {
        if ((n = recvfrom(s, & udp_pdu, pdulen, 0, (struct sockaddr * ) & fsin, & alen)) < 0) {
        printf("recvfrom error: n=%d\n", n);
        }
        //Content Registration Request 'R'			
        if (udp_pdu.type == 'R') {
        printf("Registering\n");
        registration(s, udp_pdu.data, & fsin);
        printf("%d\n", s);
        }

        // Search Content 'S'		
        if (udp_pdu.type == 'S') {
        printf("Searching\n");
        search_content(s, udp_pdu.data, & fsin);
        }

        //List Content 'O' 
        if (udp_pdu.type == 'O') {
        printf("List content\n");
        // Read from the content list and send the list to the client 		
        for (j = 0; j < max_index; j++) {
            if (list[j].head != NULL) {
            PDU_UDP spdu;
            spdu.type = 'O';
            strcpy(spdu.data, list[j].content_name);
            printf("%s\n", list[j].content_name);
            (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
            }
        }
        // Acknowledge End of Response
        PDU_UDP opdu;
        opdu.type = 'A';
        (void) sendto(s, & opdu, sizeof(opdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
        }

        //Deregister 'T'		
        if (udp_pdu.type == 'T') {
        printf("de-register\n");
        deregistration(s, udp_pdu.data, & fsin);
        }
    }
    return;
    }

    // s is socket address
    // *data is the data content_name
    // *addr is the address of the client sending/requesting
    void search_content(int s, char * data, struct sockaddr_in * addr) {
    // Search content list and return the answer:
    int j;
    int found = 0;
    int used = 999;
    LEDGER_ENTRY * use;
    LEDGER_ENTRY * head;
    int pdulen = sizeof(PDU_UDP);
    PDU_UDP spdu;
    char content_host[20];
    char ouput[100];
    char fileName[20];
    char rep[2] = ",";

    strcpy(content_host, strtok(data, rep));
    strcpy(fileName, strtok(NULL, rep));

    // loop through list until you find content_name == data.
    for (j = 0; j < max_index; j++) {
        // check if the value at the list index 
        // is equal to the requested file
        printf("%s\n", list[j].content_name);
        if (strcmp(list[j].content_name, fileName) == 0 && (list[j].head != NULL)) {
        found = 1;
        head = list[j].head;
        // loop through the linked list until you find content_name
        while (head != NULL) {
            if (head -> token < used) {
            used = head -> token;
            use = head;
            }
            head = head -> next;
        }
        break;
        }
    }
    if (found == 1) {
        spdu.type = 'S';
        strcpy(ouput, use -> content_host);
        strcat(ouput, ",");
        strcat(ouput, fileName);
        strcat(ouput, ",");
        strcat(ouput, use -> ip_addr);
        strcat(ouput, ",");
        strcat(ouput, use -> port_num);
        printf("%s\n", ouput);
        strcpy(spdu.data, ouput);
        use -> token++;
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    } else {
        spdu.type = 'E';
        strcpy(spdu.data, "File not found");
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    }
    printf("Ending\n");
    }

    // Deregister content from the server
    void deregistration(int s, char * data, struct sockaddr_in * addr) {
    int j;
    int use = -1;
    LEDGER_ENTRY * prev;
    LEDGER_ENTRY * head;
    int listIndex = 0;
    PDU_UDP spdu;
    char rep[2] = ",";
    char content_host[20];
    char file[20];
    printf("Deregistering %s\n", data);
    strcpy(content_host, strtok(data, rep));
    strcpy(file, strtok(NULL, rep));
    for (j = 0; j < max_index; j++) {
        if (strcmp(list[j].content_name, file) == 0) {
        head = list[j].head;
        prev = list[j].head;
        listIndex = 0;
        while (head != NULL) {
            printf("Usr list = %s\n", head -> content_host);
            printf("Usr given = %s\n", content_host);
            if (strcmp(head -> content_host, content_host) == 0) {
            printf("Compare content_name success\n");
            printf("List index = %d\n", listIndex);
            use = listIndex;
            if (listIndex == 0) {
                list[j].head = head -> next;
            } else {
                prev -> next = head -> next;
            }
            break;
            }
            listIndex++;
            prev = head;
            head = head -> next;
        }
        break;
        }
    }
    if (use != -1) {
        spdu.type = 'A';
        strcpy(spdu.data, "Done");
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    } else {
        spdu.type = 'E';
        strcpy(spdu.data, "Failed");
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    }
    }

    // s is socket address
    // *data is the data content_name
    // *addr is the address of the client sending/requesting
    // Register Content to the Server
    void registration(int s, char * data, struct sockaddr_in * addr) {
    // Register the content and the server of the content
    LEDGER_ENTRY * new = NULL;
    new = (LEDGER_ENTRY * ) malloc(sizeof(LEDGER_ENTRY));
    int j;
    LEDGER_ENTRY * head;
    int used = 999;

    int duplicateUser = 0;
    char rep[2] = ",";
    PDU_UDP spdu;
    char fileName[20];
    int found = 0;
    char * ip_addr = inet_ntoa(fsin.sin_addr);

    printf("Sending ip_addr address %s\n", ip_addr);
    printf("Socket %d\n", s);
    printf("Data %s\n", data);
    // get address and username from data ledger_entry
    strcpy(new -> content_host, strtok(data, rep));
    strcpy(fileName, strtok(NULL, rep));
    strcpy(new -> port_num, strtok(NULL, rep));
    strcpy(new -> ip_addr, ip_addr);

    printf("Stored Ip %s\n", new -> ip_addr);
    new -> token = 0;
    new -> next = NULL;
    // iterate list until you find content_name == data
    for (j = 0; j < max_index; j++) {
        if (strcmp(list[j].content_name, fileName) == 0) {
        head = list[j].head;
        found = 1;
        // loop through the linked list until you get to the end to add content to tail
        while (head != NULL) {
            // stops seg faults when registering content after full deregister
            if (head -> next == NULL) {
            break;
            }
            // check if username is already used
            if (strcmp(head -> content_host, data) == 0) {
            duplicateUser = 1;
            break;
            }
            head = head -> next;
        }
        if (head == NULL) { // First Content on Linked List Initialize
            list[j].head = new;
        } else { // Other Cases
            head -> next = new;
        }
        break;
        }
    }
    //File not Found
    if (found == 0) {
        strcpy(list[max_index].content_name, fileName);
        list[max_index].head = new;
        max_index++;
    }
    if (duplicateUser == 1) {
        printf("Duplicate\n");
        spdu.type = 'E';
        strcpy(spdu.data, "Duplicate user content_name"); // Duplicate File Name?
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));

    } else {
        printf("Unique\n");
        // Otherwise send acknowledgement 'A'
        spdu.type = 'A';
        strcpy(spdu.data, "Done");
        (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    }
}