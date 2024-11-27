#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

// 404 Error
#define MSG1 "Cannot find content"
// Max UDP Message Size
#define BUFLEN 100
// Max File Name Size
#define NAMESIZ 20
// Max Content
#define MAX_NUM_CON 200
// Max Content

typedef struct entry {
  char usr[NAMESIZ];
  char ip[16];
  char port[7];
  short token;
  struct entry * next;
}
ENTRY;

typedef struct {
  char name[NAMESIZ]; // Content Name
  ENTRY * head;
}
LIST;
LIST list[MAX_NUM_CON];
int max_index = 0;

// Pdu for udp communication
typedef struct {
  char type;
  char data[BUFLEN];
}
PDU;
PDU tpdu;

void search(int, char * , struct sockaddr_in * );
void registration(int, char * , struct sockaddr_in * );
void deregistration(int, char * , struct sockaddr_in * );

struct sockaddr_in fsin;

// UDP Content Indexing Service
int main(int argc, char * argv[]) {
  struct sockaddr_in sin, * p_addr; // the from address of a client	
  ENTRY * p_entry;
  char * service = "10000"; // service name or port number	
  char name[NAMESIZ], usr[NAMESIZ];
  int alen = sizeof(struct sockaddr_in); // from-address length		
  int s, n, i, len, p_sock; // socket descriptor and socket type    
  int pdulen = sizeof(PDU);
  struct hostent * hp;
  PDU rpdu;
  int j = 0;

  PDU spdu;

  for (n = 0; n < MAX_NUM_CON; n++)
    list[n].head = NULL;
  switch (argc) {
  case 1:
    break;
  case 2:
    service = argv[1];
    break;
  default:
    fprintf(stderr, "Incorrect Arguments \n Use the format: server [host] [port]\n");
  }

  memset( & sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;

  // Map service name to port number 
  sin.sin_port = htons((u_short) atoi(service));

  // Allocate socket 
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    fprintf(stderr, "can't creat socket\n");
    exit(1);
  }

  // Bind socket 
  if (bind(s, (struct sockaddr * ) & sin, sizeof(sin)) < 0)
    fprintf(stderr, "can't bind to %s port\n", service);

  while (1) {
    if ((n = recvfrom(s, & rpdu, pdulen, 0, (struct sockaddr * ) & fsin, & alen)) < 0) {
      printf("recvfrom error: n=%d\n", n);
    }
    //Content Registration Request 'R'			
    if (rpdu.type == 'R') {
      printf("Registering\n");
      registration(s, rpdu.data, & fsin);
      printf("%d\n", s);
    }

    // Search Content 'S'		
    if (rpdu.type == 'S') {
      printf("Searching\n");
      search(s, rpdu.data, & fsin);
    }

    //List Content 'O' 
    if (rpdu.type == 'O') {
      printf("List content\n");
      // Read from the content list and send the list to the client 		
      for (j = 0; j < max_index; j++) {
        if (list[j].head != NULL) {
          PDU spdu;
          spdu.type = 'O';
          strcpy(spdu.data, list[j].name);
          printf("%s\n", list[j].name);
          (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
        }
      }
      // Acknowledge End of Response
      PDU opdu;
      opdu.type = 'A';
      (void) sendto(s, & opdu, sizeof(opdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
    }

    //Deregister 'T'		
    if (rpdu.type == 'T') {
      printf("de-register\n");
      deregistration(s, rpdu.data, & fsin);
    }
  }
  return;
}

// s is socket address
// *data is the data name
// *addr is the address of the client sending/requesting
void search(int s, char * data, struct sockaddr_in * addr) {
  // Search content list and return the answer:
  int j;
  int found = 0;
  int used = 999;
  ENTRY * use;
  ENTRY * head;
  int pdulen = sizeof(PDU);
  PDU spdu;
  char usr[20];
  char ouput[100];
  char fileName[20];
  char rep[2] = ",";

  strcpy(usr, strtok(data, rep));
  strcpy(fileName, strtok(NULL, rep));

  // loop through list until you find name == data.
  for (j = 0; j < max_index; j++) {
    // check if the value at the list index 
    // is equal to the requested file
    printf("%s\n", list[j].name);
    if (strcmp(list[j].name, fileName) == 0 && (list[j].head != NULL)) {
      found = 1;
      head = list[j].head;
      // loop through the linked list until you find name
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
    strcpy(ouput, use -> usr);
    strcat(ouput, ",");
    strcat(ouput, fileName);
    strcat(ouput, ",");
    strcat(ouput, use -> ip);
    strcat(ouput, ",");
    strcat(ouput, use -> port);
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
  ENTRY * prev;
  ENTRY * head;
  int listIndex = 0;
  PDU spdu;
  char rep[2] = ",";
  char usr[20];
  char file[20];
  printf("Deregistering %s\n", data);
  strcpy(usr, strtok(data, rep));
  strcpy(file, strtok(NULL, rep));
  for (j = 0; j < max_index; j++) {
    if (strcmp(list[j].name, file) == 0) {
      head = list[j].head;
      prev = list[j].head;
      listIndex = 0;
      while (head != NULL) {
        printf("Usr list = %s\n", head -> usr);
        printf("Usr given = %s\n", usr);
        if (strcmp(head -> usr, usr) == 0) {
          printf("Compare name success\n");
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
// *data is the data name
// *addr is the address of the client sending/requesting
// Register Content to the Server
void registration(int s, char * data, struct sockaddr_in * addr) {
  // Register the content and the server of the content
  ENTRY * new = NULL;
  new = (ENTRY * ) malloc(sizeof(ENTRY));
  int j;
  ENTRY * head;
  int used = 999;

  int duplicateUser = 0;
  char rep[2] = ",";
  PDU spdu;
  char fileName[20];
  int found = 0;
  char * ip = inet_ntoa(fsin.sin_addr);

  printf("Sending ip address %s\n", ip);
  printf("Socket %d\n", s);
  printf("Data %s\n", data);
  // get address and username from data entry
  strcpy(new -> usr, strtok(data, rep));
  strcpy(fileName, strtok(NULL, rep));
  strcpy(new -> port, strtok(NULL, rep));
  strcpy(new -> ip, ip);

  printf("Stored Ip %s\n", new -> ip);
  new -> token = 0;
  new -> next = NULL;
  // iterate list until you find name == data
  for (j = 0; j < max_index; j++) {
    if (strcmp(list[j].name, fileName) == 0) {
      head = list[j].head;
      found = 1;
      // loop through the linked list until you get to the end to add content to tail
      while (head != NULL) {
        // stops seg faults when registering content after full deregister
        if (head -> next == NULL) {
          break;
        }
        // check if username is already used
        if (strcmp(head -> usr, data) == 0) {
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
    strcpy(list[max_index].name, fileName);
    list[max_index].head = new;
    max_index++;
  }
  if (duplicateUser == 1) {
    printf("Duplicate\n");
    spdu.type = 'E';
    strcpy(spdu.data, "Duplicate user name"); // Duplicate File Name?
    (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));

  } else {
    printf("Unique\n");
    // Otherwise send acknowledgement 'A'
    spdu.type = 'A';
    strcpy(spdu.data, "Done");
    (void) sendto(s, & spdu, sizeof(spdu), 0, (struct sockaddr * ) & fsin, sizeof(fsin));
  }
}
