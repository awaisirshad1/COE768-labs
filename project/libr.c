#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_PDU_DATA_SIZE 100
#define NUM_PEERS 5

struct pdu{
    char type;
    char data[MAX_PDU_DATA_SIZE];
};