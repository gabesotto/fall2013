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
	
	/* Read file into buffer */
	char window[150000];
	
	FILE *fp; 
	long file_size;
			
	//open file
	fp=fopen(argv[5], "rb"); 
		
	if(fp == NULL){
		printf("Error opening file!\n");
		exit(0);
	}
		
	//obtain file size
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	//read file into window buffer
	bzero(&window,sizeof(window));
	size_t result = fread(window, 1, file_size, fp);
	if(result != file_size){
		printf("File reading error!");
		fclose(fp);
	}
	fclose(fp);
	
	//Setup select()
	struct timeval tv;
    fd_set readfds;
    
    //set timeout
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);
	
	int nbytes = -1; //number of bytes sent
	int sws = 6; //Send Window Size
	int lar = -1; //last ack received 
	int i = 0;
	//TODO: CALCULATE LAST FRAME! PUT SPECIAL FLAG ON LAST FRAME!
	//While the EOF has not been reached.
	while(1){
		//Send window
		for(i=lar+1; i <= lar+sws; i++){
			/* Prepare packet */
			char seqNum[4];  //header
			char tempSeqNum[4];
			char payload[PAYLOADSIZE]; //payload to be sent
			
			//Prepare Header (Padded with 0's to make 3 digits ie(001, 020))
			sprintf(tempSeqNum, "%d", i); //copy int into buffer
			if(i < 10){//pad with 2 zeros
				strncpy(seqNum, "00", 4);
			}else if(i < 100){//pad with 1 zero
				strncpy(seqNum, "0", 4);
			}
			strcat(seqNum, tempSeqNum);
			//printf("header after prep: %s\n", seqNum);
			
			//prepare payload
			int beginIndex = i*1024;
			bzero(&payload, PAYLOADSIZE);
			memcpy(payload, window + beginIndex, 1024);
			int headerSize = sizeof(seqNum);        
			int packetSize = PAYLOADSIZE + headerSize;
			
			//prepare packet
			char packet[packetSize];
			bzero(&packet, packetSize);
			//TODO:Change to memcpy when writing binary file?
			//strncat(packet, seqNum, headerSize); //copy header
			//strncat(packet, payload ,PAYLOADSIZE); //copy payload
			memcpy(&packet[0], seqNum, headerSize);
			memcpy(&packet[4], payload, PAYLOADSIZE); 
			
			//printf("Size of packet: %d\n", packetSize);
			//printf("packet: %s\n", &packet[0]);
			printf("Sending packet %d\n", i);
			nbytes = sendto_(sd, packet, packetSize, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
		}//end for
		
		//TODO: Maybe put a while loop here
		//wait for and process acks
		char ack[4];
		if(select(sd+1, &readfds, NULL, NULL, &tv)<0){
			perror("Select error");
		}
		if (FD_ISSET(sd, &readfds)){
			//ACK Received process ack
			nbytes = recvfrom(sd, ack, sizeof(ack), 0, (struct sockaddr*)&remoteServAddr, &remoteServAddrLen);
			printf("Ack %s received!\n", ack);
			int ackNum = atoi(ack);
			if(ackNum == (lar + 1)){
				lar += 1;
			}
			
		}else{
			//Timeout Resend window
			printf("Timed out. Resend window!\n");
			continue;
		}
	}//end while
}//end main
		/*
		//Wait for acks, move window pointer as neccesary
		if(select(sd+1, &readfds, NULL, NULL, &tv)<0){
			perror("Select error");
		}
    
		//ACK Received process ack
		if (FD_ISSET(sd, &readfds)){
			printf("Ack received!\n");
			nbytes = recvfrom(sd, ack, sizeof(ack), 0, (struct sockaddr*)&remoteServAddr, &remoteServAddrLen);
			printf("ack: %s\n", ack);
		
		//Timeout Resend window
		}else{	
			printf("Timed out.\n");
			continue;
		}
		
	}
	
	

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
}*/



