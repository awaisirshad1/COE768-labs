#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include "libr.h"

struct IndexLedgerEntry{
    char *peerName;
    char *contentName;
};

struct IndexLedgerEntry IndexLedger[NUM_PEERS];

char *contentLedger[];
// index server needs to constantly listen for peers requests to connect to another peer over   
void registerPeer(){
    struct pdu request; 
	struct pdu response;
    struct sockaddr_in client_addr;	/* the from address of a client	*/

    
}

int main(void){


}