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
	
	/*Open log file*/
	FILE * logFp;
	logFp = fopen(argv[5], "wb");

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


	time_t now;
	struct sockaddr_in cliAddr;
	unsigned int cliLen;
	int nbytes;
	char recvmsg[1033]; //1024 + 9 = 1033
	char seqNum[4];
	char payloadSize[5];
	char flag[2];
	char payload[1024];
	char file[150000];
	int fileSize;
	int cumAckWindow[1000]; //boolean array to keep track of packets received.
	//initalize all to zero
	int z = 0;
	for(z = 0; z < sizeof(cumAckWindow); z++){
		cumAckWindow[z] = 0;
	}
	int rws = 9; //receive window size
	int lfr = -1; //last frame received
	
	while(1){
		
		/* Receive message from client */
		bzero(&recvmsg,sizeof(recvmsg));
		bzero(&seqNum, sizeof(seqNum));
		bzero(&payload, sizeof(payload));
		bzero(&flag, sizeof(flag));
		bzero(&payloadSize, sizeof(payloadSize));
		cliLen = sizeof(cliAddr);
		nbytes = recvfrom(sd, &recvmsg, sizeof (recvmsg), 0, (struct sockaddr *) &cliAddr, &cliLen);
		printf("received %d bytes\n", nbytes);

		//strip header
		memcpy(seqNum, &recvmsg[0], 3);
		seqNum[3] = '\0';
		memcpy(payloadSize, &recvmsg[3], 4);
		payloadSize[4] = '\0';
		memcpy(flag, &recvmsg[7], 2);
		flag[1] = '\0';
		
		//strip payload
		memcpy(payload, &recvmsg[9], 1024);
		
		int frameNum = atoi(seqNum);
		int payloadSizeNum = atoi(payloadSize);
		int finalPacket = atoi(flag);
		int ack;
		int cumAck;
		printf("received packet #: %d\n", frameNum);

		now = time(0);
		fprintf(logFp, "<Receive> Seq #: %d LFR: %d LAF: %d Time: %ld \n", frameNum, lfr, cumAck, now);
		
		
		/*begin processing packet and window stuff */
		if(frameNum >= lfr && frameNum <= lfr+1+rws ){ //Packet is within receive window size (rws)
			//write to payload to file buffer
			cumAckWindow[frameNum] = 1;
			memcpy(&file[frameNum*1024], payload, payloadSizeNum);
			ack = frameNum;
			
			//find next frame needed to tell client
			int i = 0;
			for(i = 0; i < sizeof(cumAckWindow)/sizeof(int); i++){
				if(cumAckWindow[i] == 0){
					cumAck = i;
					break;
				}
			}
		
			lfr = cumAck - 1;
			
			
		}else{ //packet out of window, send ack and tell where to send from
			//find next frame needed to tell client
			int i = 0;
			for(i = 0; i < sizeof(cumAckWindow)/sizeof(int); i++){
				if(cumAckWindow[i] == 0){
					cumAck = i;
					break;
				}
			}
			ack = lfr;
		}
		
		/*prepare ack*/
		char cAck[4];
		char cCumAck[4];
		char response[7];
		sprintf(cAck, "%03d", ack);
		sprintf(cCumAck, "%03d", cumAck);
		strcpy(response, cAck);
		strcat(response, cCumAck);
		printf("ack looks like: %s \n", response);


		now = time(0);
		fprintf(logFp, "<Send> Seq #: %d LFR: %d LAF: %d Time: %ld \n", frameNum, lfr, cumAck, now);
		/* Send Ack */
		nbytes = sendto_(sd, response, sizeof(response),0, (struct sockaddr *) &cliAddr, cliLen);
		
		if(finalPacket == 1){ //final packet flag
			//Write fileBuffer to file. Calculate file size cleanup
				printf("got final packet! \n");	
				fileSize = ((frameNum-1)*1024) + payloadSizeNum;
				
				FILE * outputFilePointer;
				outputFilePointer = fopen(argv[4], "wb");
	
				if(fwrite(file, 1, fileSize, outputFilePointer) != fileSize || outputFilePointer == NULL){
					printf("Error writing to file!\n");
				}
	
				fclose(outputFilePointer);
				fclose(logFp);
				break;
		}
	}
	
	close(sd);
	return 0;
}


