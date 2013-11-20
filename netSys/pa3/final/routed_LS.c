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
#define MAX 6
#define INFINITE 998

FILE* log;
struct tableEntry {
    char host;
    char target;
    int weight;
    int fromPort;
    int toPort;
    int seqNum;
    int TTL;
};

int allselected(int *selected)
{
  int i;
  for(i=0;i<MAX;i++)
    if(selected[i]==0)
      return 0;
  return 1;
}
int getIndex(char c){
	if(c == 'A') return 0;
	if(c == 'B') return 1;
	if(c == 'C') return 2;
	if(c == 'D') return 3;
	if(c == 'E') return 4;
	if(c == 'F') return 5;
	
	return -1;
}

char getChar(int i){
	if(i == 0) return 'A';
	if(i == 1) return 'B';
	if(i == 2) return 'C';
	if(i == 3) return 'D';
	if(i == 4) return 'E';
	if(i == 5) return 'F';
	return 'X';
	
}

void shortpath(struct tableEntry *graph, int tableSize, int *preced, int *distance)
{
  int cost[MAX][MAX];
  int selected[MAX]={0};
  int current=0,i,k,dc,smalldist,newdist,j,x;
  for(i=0;i<MAX;i++)
  distance[i]=INFINITE;
  selected[current]=1;
  distance[0]=0;
  current=0;
  
  //convert graph to cost[][]
  for(j = 0; j < MAX; j++){
	  for(k = 0; k < MAX; k++){
		cost[j][k] = INFINITE;
	  }
  }
  
  for(x = 0; x < tableSize; x++){
	cost[getIndex(graph[x].host)][getIndex(graph[x].target)] = graph[x].weight; 
  }
   
  while(!allselected(selected))
  {
    smalldist=INFINITE;
    dc=distance[current];
    for(i=0;i<MAX;i++)
    {
      if(selected[i]==0)
      {                                             
        newdist=dc+cost[current][i];
        if(newdist<distance[i])
        {
          distance[i]=newdist;
          preced[i]=current;
        }
        if(distance[i]<smalldist)
        {
          smalldist=distance[i];
          k=i;
        }
      }
    }
    current=k;
    selected[current]=1;
   }
}




void printTable(struct tableEntry lspTable[], int size){
	printf("%-10s%-10s%-10s%-10s%-10s\n", "Host", "Target", "Cost", "From Port", "To Port");
	int i;
	for(i = 0; i < size; i++){
		printf("%-10c%-10c%-10d%-15d%-15d\n", lspTable[i].host, lspTable[i].target, lspTable[i].weight, lspTable[i].fromPort, lspTable[i].toPort);
	}
	
	fprintf(log, "%-10s%-10s%-10s%-10s%-10s\n", "Host", "Target", "Cost", "From Port", "To Port");
	for(i = 0; i < size; i++){
		fprintf(log, "%-10c%-10c%-10d%-15d%-15d\n", lspTable[i].host, lspTable[i].target, lspTable[i].weight, lspTable[i].fromPort, lspTable[i].toPort);
	}
}

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

//return 1 if node is in routing table, 0 if it isn't
int isInTable(struct tableEntry node, struct tableEntry *graph, int tableSize){
	int i;
	for(i = 0; i < tableSize; i++){
		//match found;
		if(node.target == graph[i].target && node.host == graph[i].host){
			return 1;
		}
		//if(node.target == graph[i].host && node.host == graph[i].target){
		//	return 1;
	//	}
			
	}
	return 0;
}

int getCost(char s, char t, int precede[], int distance[]) {
    int current = getIndex(t);
	int cost = 0;
    while (current != getIndex(s)) {
       // printf("%c -> ", getChar(current));
        current = precede[current];
        cost += distance[current];
    }

    //printf("%c\n", getChar(current));
    return cost;
    
}

int main(int argc, const char* argv[]){
	
	//check command line arguments
	if(argc != 4){
		printf("Usage: ./routed_LS <router_name> <Log_file> <initialization_file>\n");
		return 1;
	}
	
	//open initialization file for reading
	FILE* fp; 
	//FILE* log;
	fp = fopen(argv[3], "r");
	log = fopen(argv[2], "w");
	if(fp == NULL){
		printf("Initialization file does not exist!\n");
		return -1;
	}
	if(log == NULL){
		printf("could not open log file!\n");
		return -1;
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
		destAddr[i].sin_family=AF_INET;
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
	printf("Connection established with all neighbors at node %c. Sleeping for 10 seconds before packet flooding\n", source[0]);
	sleep(5);
	
	//Things to keep track of:		
	struct tableEntry lspTable[numOfNeighbors];
	struct tableEntry lspRecvTable[MAXNODES];
	struct tableEntry graph[1000];
	int tableSize = 0; //number of rows in graph
	int killCount = 0;
	
	int seqNum = 0;
	char recvBuffer[512];
	char sendBuffer[512];
	char sendBuffer2[512];

	//initalize table for child to send
	for(i = 0; i < numOfNeighbors; i++){
		lspTable[i].host = source[i];
		lspTable[i].target = dest[i];
		lspTable[i].weight = cost[i];
		lspTable[i].fromPort = sourcePort[i];
		lspTable[i].toPort = destPort[i];
		lspTable[i].seqNum = seqNum;
		lspTable[i].TTL = 2;
		
		graph[i].host = source[i];
		graph[i].target = dest[i];
		graph[i].weight = cost[i];
		graph[i].fromPort = sourcePort[i];
		graph[i].toPort = destPort[i];
		graph[i].seqNum = seqNum;
		graph[i].TTL = 3;
		
		tableSize++;
	}
	
	//serialize LSP packet for sending
	memcpy(&sendBuffer[0], &lspTable[0], sizeof(lspTable));
		
	//set up sockets
	for(i = 0; i < numOfNeighbors; i++){
		if(maxfd <= sockfd[i]){
			maxfd = sockfd[i];
		}
		FD_SET(sockfd[i], &masterfds);
		lspTable[i].seqNum = seqNum;
	}	
	
	pid_t childPID;

    childPID = fork();
	
	if(childPID >= 0) // fork was successful
    {
        if(childPID == 0) // child process
        {	
			for(;;){	
				sleep(5);
				int j;
				for(j = 0; j < numOfNeighbors; j++){
					nbytes = send(sockfd[j], sendBuffer, sizeof(lspTable), 0);
					if(nbytes <= 0){
						printf("Error on send()\n");
						return -1;
					}
				}
			}
        }
        else //Parent process
        {
				for(;;){
					killCount++;
					
					//listen for packets from neighbors
					readfds = masterfds;
					if(select(maxfd+1, &readfds, NULL, NULL, NULL) < 0){
						printf("Select() Error!\n");
						return 1;
					}
					
					for(i = 0; i < numOfNeighbors; i++){
						bzero(&recvBuffer,sizeof(recvBuffer));
						bzero(&lspRecvTable,sizeof(lspRecvTable));
			
						if (FD_ISSET(sockfd[i], &readfds)){
							
							//Got LSP from neighbor
							nbytes = recv(sockfd[i], recvBuffer, sizeof(recvBuffer), 0);
							if(nbytes <= 0){
								printf("Error on recv()\n");
								return -1;
							}
							
							//check for end condition
							if(recvBuffer[0] == 'X'){
								printf("CYA!\n");
								return 0;
							}
							
							//deserialize
							int rows = nbytes / sizeof(struct tableEntry);
							memcpy(&lspRecvTable[0], &recvBuffer[0], nbytes);
							
							if(rows > 4){
								break;
							}
						
							//decrement TTL
							int k;
							for(k = 0; k < rows; k++){
								lspRecvTable[k].TTL--;
							}
						
							//send it all neighbors except who sent it.
							int j;
							for(j = 0; j < numOfNeighbors; j++){
								//if TTL is 0 dont send;
								//printf("lspRecvTable[0].TTL = %d\n", lspRecvTable[0].TTL);
								if(lspRecvTable[0].TTL <= 0){
									break;
								}	
								if(i != j){
									
									//serialize LSP packet for sending
									bzero(&sendBuffer2, sizeof(sendBuffer2));
									memcpy(&sendBuffer2[0], &lspRecvTable[0], nbytes);
									
									int nbytes2=0;
									nbytes2 = send(sockfd[j], sendBuffer2, nbytes, 0);
									if(nbytes2 <= 0){
										printf("Error on send()\n");
										return -1;
									}
								}
							}	
		
							//fill graph with all known nodes
							for(j = 0; j < rows; j++){
								//not in table, so add it.
								if(isInTable(lspRecvTable[j], graph, tableSize) == 0){
									graph[tableSize] = lspRecvTable[j];
									tableSize++;
								}
							}
												
							//run djikstras
							int preced[MAX]={5},distance[MAX];
							shortpath(graph, tableSize, preced, distance);							
							
							fprintf(stdout, "routing table: \n");
							fprintf(log, "routing table: \n");
							
							printTable(graph, tableSize);
							
							fprintf(stdout, "result from djikstras:\n");
							fprintf(log, "result from djikstras:\n");
							
							for(i=0;i<MAX;i++){
								fprintf(stdout, "%c:%d ",getChar(i), distance[i]);
								fprintf(log, "%c:%d ",getChar(i), distance[i]);
							}
								fprintf(stdout, "\n");
								fprintf(log, "\n");
							
							
							if(killCount >= 15){
								printf("CYA!\n");
								fclose(log);
								fclose(fp);
								int nbytes2=0;
								sendBuffer2[0] = 'X';
								for(j = 0; j < numOfNeighbors; j++){
									nbytes2 = send(sockfd[j], sendBuffer2, nbytes, 0);
									if(nbytes2 <= 0){
										printf("Error on send()\n");
										return 0;
									}
								}
								return 0;
							}
							
								
						}//end FD_ISSET
					}//END CONNECTION LOOP
				}//END PARENT FOR LOOP
			}//END PARENT IF
		}//END FORK
	}//END MAIN	
