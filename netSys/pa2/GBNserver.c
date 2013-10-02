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


void stripHeader(char packet[], char head[]);

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
	char recvmsg[1028];
	bzero(recvmsg,sizeof(recvmsg));
	cliLen = sizeof(cliAddr);
	nbytes = recvfrom(sd, &recvmsg, sizeof (recvmsg), 0, (struct sockaddr *) &cliAddr, &cliLen);
	
	printf("client says %s \n", recvmsg);
	
	char ack[4];
	char payload[1024];
	stripHeader(recvmsg, ack);
	memmove(recvmsg, recvmsg+3, sizeof(recvmsg));
	memcpy(payload, recvmsg, sizeof(payload));
	//printf("header stripped: %s \n", ack);
	//printf("payload stripped; %s \n", payload);
	
	/* Send Ack */
	nbytes = sendto_(sd, ack, sizeof(ack),0, (struct sockaddr *) &cliAddr, cliLen);
	printf("nbytes sent: %d\n", nbytes);
}

//Remove header from packet, returns int version of header
void stripHeader(char packet[], char head[]){
	int i = 0;
	for(i = 0; i < 3; i++){
		head[i] = packet[i];
	}
}

