#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define MAX_PDU_DATA_SIZE

struct pdu{
    char type;
    char data[MAX_PDU_DATA_SIZE];
};


// index server needs to constantly listen for peers requests to connect to another peer over   
void registerPeer(void){

}

int main(void){

}