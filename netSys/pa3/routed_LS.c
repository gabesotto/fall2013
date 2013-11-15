#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>	

#define MAXNODES 10
int checkConnections(int connected[], int numOfNeighbors){
	//check if connectted to all neighbors
	int i;
	for(i = 0; i < numOfNeighbors; i++){
		if(connected[i] == 0){
			return 1;
		}
		if(connected[i] == 1 && i == (numOfNeighbors - 1)){
			return 0;
		}
			
	}
	return 1;
}

struct tableEntry {
    char host;
    char target;
    int weight;
    int fromPort;
    int toPort;
    int seqNum;
    int TTL;
};

int main(int argc, const char* argv[]){
	
	//check command line arguments
	if(argc != 3){
		printf("Usage: ./routed_LS <router_name> <initialization_file>\n");
		return 1;
	}
	
	//open initialization file for reading
	FILE* fp; 
	fp = fopen(argv[2], "r");
	if(fp == NULL){
		printf("Initialization file does not exist!\n");
		return 1;
	}
	
	//read from init file
	char buffer[256];
	char source[MAXNODES], dest[MAXNODES];
	int sourcePort[MAXNODES], destPort[MAXNODES], cost[MAXNODES];
	int numOfNeighbors = 0;
	while (fscanf(fp, "%s", buffer) != EOF) {
		if(buffer[1] == argv[1][0]){
			sscanf(buffer, "<%c,%d,%c,%d,%d>", &source[numOfNeighbors], &sourcePort[numOfNeighbors], &dest[numOfNeighbors], &destPort[numOfNeighbors], &cost[numOfNeighbors]); 
			numOfNeighbors++;
		}
	}
	
	//blindly try to connect to all neighbors.
	int sockfd[numOfNeighbors];
	int maxfd = 0;
	int nbytes;	
	struct sockaddr_in destAddr[numOfNeighbors];
	struct sockaddr_in sourceAddr[numOfNeighbors];
	int connected[numOfNeighbors];
	int needNeighbors = 1;
    fd_set readfds, masterfds;
	FD_ZERO(&readfds);
	FD_ZERO(&masterfds);
	int i = 0;
	for(i = 0; i < numOfNeighbors; i++){
		connected[i] = 1;
	}
	for(i = 0; i < numOfNeighbors; i++){	
		sockfd[i]=socket(AF_INET,SOCK_STREAM,0);
		
		//local
		bzero(&sourceAddr[i],sizeof(sourceAddr[i]));
		sourceAddr[i].sin_family = AF_INET;
		sourceAddr[i].sin_addr.s_addr=inet_addr("127.0.0.1");
		sourceAddr[i].sin_port=htons(sourcePort[i]);

		
		if(bind(sockfd[i], (struct sockaddr *)&sourceAddr[i], sizeof(sourceAddr[i]))<0){
			printf("Could not bind to port %c:%d\n", source[i], sourcePort[i]);
			return 1;
		}
		
		//remote
		bzero(&destAddr[i],sizeof(destAddr[i]));
		destAddr[i].sin_family = AF_INET;
		destAddr[i].sin_addr.s_addr=inet_addr("127.0.0.1");
		destAddr[i].sin_port=htons(destPort[i]);
		
		
		if(connect(sockfd[i], (struct sockaddr *)&destAddr[i], sizeof(destAddr[i])) < 0){ 
		//connect failed open listening port
			maxfd = sockfd[i];
			listen(sockfd[i], 1);
			FD_SET(sockfd[i], &masterfds);
			connected[i] = 0;
		}else{
			printf("Connection!: %c:%d -> %c:%d\n", source[i], ntohs(sourceAddr[i].sin_port), dest[i], ntohs(destAddr[i].sin_port));
		}
	}
	
	//check if there are neighbors to connect to
	needNeighbors = checkConnections(connected, numOfNeighbors);

	//Begin listening for neighbors you didn't connect to
	while(needNeighbors){ 
		readfds = masterfds; //copy it
		if(select(maxfd+1, &readfds, NULL, NULL, NULL) < 0){
			printf("Select() Error!\n");
			return 1;
		}
	
		// run through connections and listen for neighbors
		for(i = 0; i < numOfNeighbors; i++){
			 if (FD_ISSET(sockfd[i], &readfds)) {
				socklen_t addrLength = sizeof destAddr[i];
				int newfd = accept(sockfd[i], (struct sockaddr *)&destAddr[i], &addrLength);
				if(newfd < 0){
					printf("accept error!\n");
				}else{
					FD_CLR(sockfd[i], &masterfds);
					sockfd[i] = newfd;
					connected[i] = 1;
					if (newfd > maxfd) {    // keep track of the max
						maxfd = newfd;
                    }
                    printf("Connection!: %c:%d -> %c:%d\n", source[i], ntohs(sourceAddr[i].sin_port), dest[i], ntohs(destAddr[i].sin_port));
				}
			}
		}
		//check if connected to all neighbors
		needNeighbors = checkConnections(connected, numOfNeighbors);
	}
	
	//Connected to all neighbors!
	printf("Connection established with all neighbors at node %c.\n", source[0]);
	
	//Things to keep track of:		
	struct tableEntry lspTable[numOfNeighbors];
	struct tableEntry lspRecvTable[MAXNODES];
	//struct tableEntry routingTable[MAXNODES];
	int seqNum = 0;
	char recvBuffer[512];
	char sendBuffer[512];

	//initalize table for neighbors
	for(i = 0; i < numOfNeighbors; i++){
		lspTable[i].host = source[i];
		lspTable[i].target = dest[i];
		lspTable[i].weight = cost[i];
		lspTable[i].fromPort = sourcePort[i];
		lspTable[i].toPort = destPort[i];
		lspTable[i].seqNum = seqNum;
	}
	
	//serialize LSP packet for sending
	memcpy(&sendBuffer[0], &lspTable[0], sizeof(lspTable));
		
	for(;;){
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_ZERO(&masterfds);
	
		//set up listening sockets
		for(i = 0; i < numOfNeighbors; i++){
			maxfd = sockfd[i];
			FD_SET(sockfd[i], &masterfds);
			lspTable[i].seqNum = seqNum;
		}
		
		//Listen for packets for 5 seconds.
		readfds = masterfds;
		if(select(maxfd, &readfds, NULL, NULL, &tv) < 0){
			printf("Select() Error!\n");
			return 1;
		}
	
		seqNum++;
		for(i = 0; i < numOfNeighbors; i++){
			bzero(&recvBuffer,sizeof(recvBuffer));
			bzero(&lspRecvTable,sizeof(lspRecvTable));
			
			if (FD_ISSET(sockfd[i], &readfds)){
							
				//Got LSP from neighbor
				nbytes = recv(sockfd[i], recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
				if(nbytes <= 0){
					printf("Error on recv()\n");
					return -1;
				}
				//deserialize packet
				memcpy(&lspRecvTable[0], &recvBuffer[0], nbytes);
				printf("Got packet from %c\n", lspRecvTable[0].host);
				
				//run djikstras
				
				//Send it to other neighbors
				int j = 0;
				for(j = 0; j < numOfNeighbors; j++){
					if(j != i){
						nbytes = send(sockfd[j], recvBuffer, nbytes, 0);
						if(nbytes <= 0){
							printf("Error on send()\n");
							return -1;
						}
					}
				}
				//break; //maybe? maybe not?
				
			//At timeout transmit new LSP.
			}else{
				nbytes = send(sockfd[i], sendBuffer, sizeof(lspTable), 0); 
				if(nbytes <= 0){
					printf("Error on send()\n");
					return -1;
				}
			}
		}
	}
}
