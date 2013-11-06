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
	int sourcePort[MAXNODES], destPort[MAXNODES];
	int numOfNeighbors = 0;
	while (fscanf(fp, "%s", buffer) != EOF) {
		if(buffer[1] == argv[1][0]){
			sscanf(buffer, "<%c,%d,%c,%d>", &source[numOfNeighbors], &sourcePort[numOfNeighbors], &dest[numOfNeighbors], &destPort[numOfNeighbors]); 
			numOfNeighbors++;
		}
	}
	
	//blindly try to connect to all neighbors.
	int sockfd[numOfNeighbors];
	int nbytes;	
	struct sockaddr_in destAddr[numOfNeighbors];
	struct sockaddr_in sourceAddr[numOfNeighbors];
	
	int i = 0;
	for(i = 0; i < numOfNeighbors; i++){	
		sockfd[i]=socket(AF_INET,SOCK_STREAM,0);
		
		//local
		bzero(&sourceAddr[i],sizeof(sourceAddr[i]));
		sourceAddr[i].sin_family = AF_INET;
		sourceAddr[i].sin_addr.s_addr=inet_addr("127.0.0.1");
		sourceAddr[i].sin_port=htons(sourcePort[i]);
		
		if(bind(sockfd[i], (struct sockaddr *)&sourceAddr[i], sizeof(sourceAddr[i]))<0){
			printf("Could not bind to node %c:%d\n", source[i], sourcePort[i]);
			return 1;
		}
		
		//remote
		bzero(&destAddr[i],sizeof(destAddr[i]));
		destAddr[i].sin_family = AF_INET;
		destAddr[i].sin_addr.s_addr=inet_addr("127.0.0.1");
		destAddr[i].sin_port=htons(destPort[i]);
		
		if(connect(sockfd[i], (struct sockaddr *)&destAddr[i], sizeof(destAddr[i])) < 0){
			printf("Could not connect to node %c:%d\n", dest[i], destPort[i]);
		}
	}
	
	//open listening ports for neighbors you didn't connect to. 
	
}
