/* GBNclient.c */
/* This is a sample UDP client/sender using "sendto_.h" to simulate dropped packets.  */
/* This code will not work unless modified. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   /* memset() */
#include <sys/time.h> /* select() */
#include <signal.h>
#include <unistd.h>
#include "sendto_.h"
#include "header.h"

#define PAYLOADSIZE 1024
int main(int argc, char *argv[]) {
    
	/* check command line args. */
	if(argc<7)
	{
		printf("usage : %s <server_ip> <server_port> <error rate> <random seed> <send_file> <send_log> \n", argv[0]);
		exit(1);
	}

	/* Note: you must initialize the network library first before calling sendto_().  The arguments are the <errorrate> and <random seed> */
	init_net_lib(atof(argv[3]), atoi(argv[4]));
	printf("error rate : %f\n",atof(argv[3]));

	/* socket creation */
	int sd;
	if((sd = socket(PF_INET,SOCK_DGRAM,0))<0){
		printf("%s: cannot create socket \n",argv[0]);
		exit(1);
	}

	/* get server IP address (input must be IP address, not DNS name) */
	struct sockaddr_in remoteServAddr;
	unsigned int remoteServAddrLen;
	bzero(&remoteServAddr,sizeof(remoteServAddr));        //zero the struct
	remoteServAddr.sin_family = AF_INET;                 //address family
	remoteServAddr.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remoteServAddr.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address
	printf("%s: sending data to '%s:%s' \n", argv[0], argv[1], argv[2]);


	/* Send packet */
	int nbytes;
	char seqNum[4];
	char payload[PAYLOADSIZE];
	strcpy(seqNum, "001");
	strcpy(payload, "To be or not to be");
	int headerSize = sizeof(seqNum);
	int packetSize = PAYLOADSIZE + headerSize;
	char packet[packetSize];
	
	bzero(&packet, packetSize);
	//Change to memcpy when writing binary file
	strncat(packet, seqNum, headerSize); //copy header
	strncat(packet, payload ,PAYLOADSIZE); //copy payload
	
	//printf("Size of packet: %d\n", packetSize);
	//printf("packet: %s\n", packet);
	nbytes = sendto_(sd, packet, packetSize, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
	
	//Wait for ack
	struct timeval tv;
    fd_set readfds;
    
    //set timeout
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);

	char ack[4];
	
    if(select(sd+1, &readfds, NULL, NULL, &tv)<0){
		perror("Select error");
	}
    
    if (FD_ISSET(sd, &readfds)){
		//ACK Received process ack
        printf("Ack received!\n");
        nbytes = recvfrom(sd, ack, sizeof(ack), 0, (struct sockaddr*)&remoteServAddr, &remoteServAddrLen);
		printf("ack: %s\n", ack);
    }else{
		//Timeout Resend window
        printf("Timed out.\n");
	}
}



