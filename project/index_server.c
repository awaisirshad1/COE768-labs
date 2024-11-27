#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include "libr.h"
#define FILE_NAME_MAX 20
#define UPD_MAX_BUF 100
#define NOT_FOUND "Content Not Found"
#define MAX_LEDGER_ENTRIES 50

typedef truct ledger_entry{
    char peerName[FILE_NAME_MAX];
    char ip_addr[16];
    char port_num[7];
    int token;
    struct ledger_entry * next;
} LEDGER_ENTRY;

typedef srtuct { 
    char 
} LEDGER;
struct IndexLedgerEntry IndexLedger[NUM_PEERS];

char *contentLedger[];
// index server needs to constantly listen for peers requests to connect to another peer over   
void registerPeer(){
    struct pdu request; 
	struct pdu response;
    struct sockaddr_in client_addr;	/* the from address of a client	*/
}

int main(void){
    int socket desc;
    struct sockaddr_in server_address, client_address;
    
    return (0);
}