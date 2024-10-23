/* A simple echo client using TCP */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>



#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		5	/* buffer length */

int main(int argc, char **argv)
{
	int 	n, i, bytes_to_read;
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*host, *bp, rbuf[BUFLEN], sbuf[BUFLEN];

	switch(argc){
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}
	// printf("Reading hello message:\n");
	// n = read(sd, rbuf, BUFLEN-1);
	// if(n>0){
	//   rbuf[n]='\0';
	//   printf("Received message from server: %s\n",rbuf);
	// }
	// else{
	//   fprintf(stderr, "failed to receive message\n");
	// }
	
	// printf("Transmit: \n");
	// while(n=read(0, sbuf, BUFLEN)){	// get user message 
	//   write(sd, sbuf, n);		// send it out 
	//   printf("Receive: \n");
	//   bp = rbuf;
	//   bytes_to_read = n;
	//   while ((i = read(sd, bp, bytes_to_read)) > 0){
	// 	bp += i;
	// 	bytes_to_read -=i;
	//   }
	//   write(1, rbuf, n);
	//   printf("Transmit: \n");
	// }

	printf("waiting on message from server");
	while (1) { // Infinite loop to keep receiving messages
    	// Read server response
		n = read(sd, rbuf, BUFLEN - 1);
		if (n > 0) {
			rbuf[n] = '\0'; // Null-terminate the received message
			write(1, rbuf, n); // Output the server message

			// Check if the message indicates to exit
			if (strcmp(rbuf, "hello") == 0) { // Example condition to exit
				printf("Server requested to exit. Closing connection.\n");
				break; // Exit the loop if server sends "exit"
			}
		} else if (n == 0) {
			// Server closed the connection
			printf("Server closed the connection.\n");
			break; // Exit the loop
		} else {
			perror("Error reading from server"); // Handle read error
			break; // Exit on error
		}
	}
        
	close(sd);
	return(0);
}
