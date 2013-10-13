/* GBNclient.c */
/* This is a sample UDP client/sender using "sendto_.h" to simulate dropped packets.  */
/* This code will not work unless modified. */

#include <time.h>
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
	
	/*Open log file*/
	FILE * logFp;
	logFp = fopen(argv[6], "wb");
	
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
	
	time_t now;
	int nbytes = -1; //number of bytes sent
	int sws = 6; //Send Window Size
	int lar = -1; //last ack received 
	int i = 0;
	int numberOfPackets = (result / PAYLOADSIZE) + 1;
	int sizeOfFinalPacket = result % 1024;
	
	//printf("size of file: %d; number of packets: %d; size of final packet: %d;", (int)result, numberOfPackets, sizeOfFinalPacket);
	
	char seqNum[4];  //seqNumber
	char payloadSize[5]; //size of data in payload
	char flag[2]; //flag to indicate final packet
	char payload[PAYLOADSIZE]; //payload to be sent
	char head[9];
	char closingCount = 0; //counter to keep track of lost final ack.

	//While the EOF has not been reached.
	while(1){
		//Send window
		for(i=lar+1; i <= lar+sws && i <= numberOfPackets; i++){
			/* Prepare packet */
			//clear buffers
			bzero(&seqNum, sizeof(seqNum));
			bzero(&payloadSize, sizeof(payloadSize));
			bzero(&flag, sizeof(flag));
			bzero(&head, sizeof(head));
			
			//Prepare seqNum
			sprintf(seqNum, "%03d", i);
			
			//prepare payload size and packet flag
			if(i == numberOfPackets){ //case for final packet
				sprintf(payloadSize,"%04d", sizeOfFinalPacket);
				strcpy(flag, "1");
				closingCount++;
			}else{
				sprintf(payloadSize, "%04d", PAYLOADSIZE);
				strcpy(flag, "0");
			}
			strcat(head, seqNum);
			strcat(head, payloadSize);
			strcat(head, flag);
					
			//prepare payload
			int beginIndex = i*1024;
			bzero(&payload, PAYLOADSIZE);
			memcpy(payload, window + beginIndex, 1024);
			int headerSize = sizeof(head);        
			int packetSize = PAYLOADSIZE + headerSize;
			
			//prepare packet
			char packet[packetSize];
			bzero(&packet, packetSize);
			memcpy(&packet[0], head, headerSize);
			memcpy(&packet[9], payload, PAYLOADSIZE); 
		
		
			now = time(0);
			fprintf(logFp, "<Send> Seq #: %d LAR: %d LFS: %d Time: %ld \n", atoi(seqNum), lar, atoi(seqNum)-1, now );
			nbytes = sendto_(sd, packet, packetSize, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
		}//end for
		
		if(closingCount >= 10){
			printf("final ack lost, closing! \n");
			break;
		}
		
		//wait for and process acks
		char ack[7];
		char cumAck[4];
		char frameNumber[4];
		while(1){
		
			//Setup select()
			struct timeval tv;
			fd_set readfds;
    
			//set timeout
			tv.tv_sec = 0;
			tv.tv_usec = 50000;
			FD_ZERO(&readfds);
			FD_SET(sd, &readfds);
			
			
			bzero(&ack, sizeof(ack));
			bzero(&cumAck, sizeof(cumAck));
			bzero(&frameNumber, sizeof(frameNumber));
			if(select(sd+1, &readfds, NULL, NULL, &tv)<0){
				perror("Select error");
				exit(0);
			}
			
			if (FD_ISSET(sd, &readfds)){
				//ACK Received process ack
				nbytes = recvfrom(sd, ack, sizeof(ack), 0, (struct sockaddr*)&remoteServAddr, &remoteServAddrLen);
				
				printf("Ack %s received!\n", ack);
				memcpy(frameNumber, &ack[0], 3);
				frameNumber[3] = '\0';
				memcpy(cumAck, &ack[4], 4);
			
				int ackNum = atoi(frameNumber);
				int cumAckNum = atoi(cumAck);
				printf("Got ack %d and cumAck %d \n", ackNum, cumAckNum);
				lar = cumAckNum - 1;
				
				now = time(0);
				fprintf(logFp, "<Receive> <Seq #: %d> <LAR: %d> <LFS: %d> <Time: %ld> \n", ackNum, lar, atoi(seqNum), now );
				if(ackNum == numberOfPackets){
					printf("DONE!\n");
					fclose(logFp);
					exit(0);
				}
			
			}else{
				//Timeout Resend window
				printf("Timed out. Resend window!\n");
				break;
			}
		}
	}//end while
}//end main




