/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
                                                                                
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>

#define	BUFSIZE 100
#define DATASIZE_MAX 100

#define	MSG		"what time is it?\n"


struct pdu {
	char type; 
	char data[100];

};

/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *-------------------------------MSG-----------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "3000";
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/

	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		service = argv[2];
		/* FALL THROUGH */
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	// s = socket(AF_INET, SOCK_DGRAM, 0);


	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
                                                                                
    /* Map service name to port number */
        sin.sin_port = htons((u_short)atoi(service));
                                                                                
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
                }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE ){
		fprintf(stderr, "Can't get host entry \n");
		exit(1);
	}
                                                                                
                                                                                
    /* Allocate a socket */
        s = socket(PF_INET, SOCK_DGRAM, 0);
        if (s < 0){
		fprintf(stderr, "Can't create socket \n");
		exit(1);
	}
	
                                                                                
    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		fprintf(stderr, "Can't connect to %s %s \n", host, service);
		exit(1);
	}


	(void) write(s, MSG, strlen(MSG));

	/* Read the time */

	n = read(s, (char *)&now, sizeof(now));
	if (n < 0)
		fprintf(stderr, "Read failed\n");
	write(1, now, n);
	exit(0);
}
