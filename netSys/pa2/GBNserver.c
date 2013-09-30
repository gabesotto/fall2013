/* GBNserver.c */
/* This is a sample UDP server/receiver program */
/* This code will not work unless modified. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <stdlib.h>
#include <time.h>
#include "sendto_.h"

int main(int argc, char *argv[]) {

	/* check command line args. */
	if(argc<6) {
		printf("usage : %s <server_port> <error rate> <random seed> <send_file> <send_log> \n", argv[0]);
		exit(1);
	}

	/* Note: you must initialize the network library first before calling sendto_().  The arguments are the <errorrate> and <random seed> */
	init_net_lib(atof(argv[2]), atoi(argv[3]));
	printf("error rate : %f\n",atof(argv[2]));

	/* socket creation */
	int sd; 
	if((sd=socket(PF_INET,SOCK_DGRAM,0))<0){
		printf("%s: cannot open socket \n",argv[0]);
		exit(1);
	}

	/* bind server port to "well-known" port whose value is known by the client */
	struct sockaddr_in servAddr;
	bzero(&servAddr,sizeof(servAddr));                    //zero the struct
	servAddr.sin_family = AF_INET;                   //address family
	servAddr.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	servAddr.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	if(bind(sd, (struct sockaddr *)&servAddr, sizeof(servAddr))<0){
		printf("%s: cannot to bind port number %s \n",argv[0], argv[1]);
		exit(1); 
	}

	/* Receive message from client */
	struct sockaddr_in cliAddr;
	unsigned int cliLen;
	int nbytes;
	char recvmsg[100];
	bzero(recvmsg,sizeof(recvmsg));
	cliLen = sizeof(cliAddr);
	nbytes = recvfrom(sd, &recvmsg, sizeof (recvmsg), 0, (struct sockaddr *) &cliAddr, &cliLen);
	
	/* Respond using sendto_ in order to simulate dropped packets */
	char response[] = "respond this";
	nbytes = sendto_(sd, response, strlen(response),0, (struct sockaddr *) &cliAddr, sizeof(cliLen));
	printf("client says %s \n", recvmsg);
}

