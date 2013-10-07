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

	struct sockaddr_in cliAddr;
	unsigned int cliLen;
	int nbytes;
	char recvmsg[1028];
	char packetNum[4];
	char payload[1024];
	
	while(1){
		
		/* Receive message from client */
		bzero(recvmsg,sizeof(recvmsg));
		bzero(packetNum, sizeof(packetNum));
		bzero(payload, sizeof(payload));
		cliLen = sizeof(cliAddr);
		nbytes = recvfrom(sd, &recvmsg, sizeof (recvmsg), 0, (struct sockaddr *) &cliAddr, &cliLen);
		//printf("received %d bytes\n", nbytes);
		//printf("client says %s \n", recvmsg);
		
		stripHeader(recvmsg, packetNum);
		memmove(recvmsg, &recvmsg[3], sizeof(recvmsg));
		memcpy(payload, recvmsg, sizeof(payload));
		//printf("header stripped: %s \n", ack);
		//printf("payload stripped; %s \n", payload);
		int frameNum = atoi(packetNum);
		printf("received packet #: %d\n", frameNum);
		
		if(){ //Packet is within receive window size (rws)
			//write to payload to file buffer
			//send ack back
		}else if(){ //final packet flag
			break;
		}else{ //throw away packet
			continue;
		}

		/* Send Ack */
		nbytes = sendto_(sd, packetNum, sizeof(packetNum),0, (struct sockaddr *) &cliAddr, cliLen);
	}
	
	close(sd);
	return 0;
}

//Remove header from packet
void stripHeader(char packet[], char head[]){
	int i = 0;
	for(i = 0; i < 3; i++){
		head[i] = packet[i];
	}
}

