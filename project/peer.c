#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include "libr.h"

// name of this peer
static const char* PEER_NAME;

int tcpPassiveSocketNum;


void registerThisPeer(){
    struct sockaddr_in reg_addr;
    s = socket(AF_INET, SOCK_STREAM,0);
    reg_addr.sin_family 
}

int main(int argc, char *argv[])
{
    exit(0);
}
