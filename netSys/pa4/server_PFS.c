/*
 * Author: Gabe Russotto
 * PA4 - Peer File System (See ReadMe for details)
 * 12.9.13
 * Network Systems
 * 
 */
 
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

#define MAXCLIENTS 5
#define MAXBUFSIZE 10000
#define MAXFILES 32

struct listEntry {
	char command;
	char filename[16];
	int filesize;
	char clientName[16];
	char clientIp[16];
	int port;
};

//remove all files with name client from list...
int removeFiles(char name[],struct listEntry masterFileList[], int size){
	struct listEntry temp[size];
	int i;
	int numOfFiles = 0;
	for(i = 0; i < size; i++){
		if(strcmp(name, masterFileList[i].clientName) != 0){
			temp[numOfFiles] = masterFileList[i];
			numOfFiles++;
		}
	}
	for(i=0; i < numOfFiles; i++){
		masterFileList[i] = temp[i];
	}
	
	return numOfFiles;
}

//returns 0 if name is already a client on the master file list of size size.
int isClient(char name[], struct listEntry masterFileList[], int size){
	int i;
	for(i = 0; i < size; i++){
		if(strcmp(masterFileList[i].clientName, name) == 0){
			return 1;
		}
	}
	return 0;
}

void printList(struct listEntry fileList[], int size){
	int i;
	for(i = 0; i < size; i++){
		printf("file: %s; size: %d; client: %s; ip: %s; port: %d\n", fileList[i].filename, 
			fileList[i].filesize, fileList[i].clientName, fileList[i].clientIp, fileList[i].port);
	}
}

 int main(int argc, const char* argv[]){
	int listener, fdmax, newfd, nbytes;
	struct sockaddr_in myAddr, clientAddr;
	char recvBuff[MAXBUFSIZE];
	char sendBuff[MAXBUFSIZE];
	char command;
	struct listEntry masterFileList[MAXFILES];
	int numOfClients = 0;
	int numOfFiles = 0;
	
	fd_set master;    // master file descriptor list
    fd_set readfds;  // temp file descriptor list for select() 
    socklen_t addrlen;
	
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&readfds);
	
	//check command line arguments
	if(argc !=2){
		printf("Usage: ./server_PFS <Port_Number>\n");
		return -1;
	}
	
	//Open Listening port for clients
	listener=socket(AF_INET,SOCK_STREAM,0);
	if(listener < 0){
		printf("Error creating socket!\n");
		return -1;
	}
	
	bzero(&myAddr,sizeof(myAddr));              //zero the struct
	myAddr.sin_family = AF_INET;                //address family
	myAddr.sin_port = htons(atoi(argv[1]));     //htons() sets the port # to network byte order
	myAddr.sin_addr.s_addr = INADDR_ANY;        //supplies the IP address of the local machine
	
	if(bind(listener, (struct sockaddr *)&myAddr, sizeof(myAddr))<0){
		printf("Could not bind to port %s\n", argv[1]);
		return -1;
	}
	
	if(listen(listener, MAXCLIENTS) == -1) {
        printf("listen fail!\n");
        return -1;
    }
    
    // add the listener to the master set
    FD_SET(listener, &master);
    
    // keep track of the biggest file descriptor
    fdmax = listener;
	
	// main loop
    for(;;) {
		readfds = master; // copy it
        if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
            printf("select fail!\n");
            return -1;
        }
        
        //run through all connections
         int i;
         for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &readfds)) { // we got one!!
				
				//clear buffers for saftey 
				bzero(&sendBuff, sizeof(sendBuff));
				bzero(&recvBuff, sizeof(recvBuff));
				
				//new connection
                if (i == listener) {
					addrlen = sizeof clientAddr;
                    newfd = accept(listener, (struct sockaddr *)&clientAddr, &addrlen);
                    if(newfd < 0){
						printf("error accepting client\n");
					} else {
						//client has connected
						FD_SET(newfd, &master); //add it to master set
						if(newfd > fdmax){ //keep track of maxfd
							fdmax = newfd;
						}
						
						printf("Server: New client has connected.\n"); 
						numOfClients++;
					}
				} else {
				// handle data from an existing client
					if ((nbytes = recv(i, recvBuff, sizeof(recvBuff), 0)) <= 0) {
						if(nbytes == 0){
							//closed Connection
							printf("Server: Client disconnected improperly, file list is probably corrupt!\n");
						}else{
							printf("Server: error on recv()\n");
						}
						close(i);
						FD_CLR(i, &master);
					} else {
						//we got data from a client		
								
						//deserialize packet
						int edgeCase = 0;
						int rows = (nbytes)/sizeof(struct listEntry);
						struct listEntry temp[rows];
						memcpy(temp, &recvBuff[0], nbytes);
						command = temp[0].command;
						
						if(command == 'a'){				
							//check if client name is in use.
							char response;
							if(isClient(temp[0].clientName, masterFileList, numOfFiles) == 0){
								printf("Server: Accepting client %s!\n", temp[0].clientName);
								response = 'y';
								
								if(strcmp(temp[0].filename, "") == 0){
									edgeCase = 1;
								}
								
								if(edgeCase == 0){
									//add files to master list
									int i = 0;
									for(i = 0; i < rows; i++){
										masterFileList[numOfFiles] = temp[i];
										numOfFiles++;
									}
								}
								
								printf("Server: Master file list updated, sending to all clients.\n");
								//send master list to all clients
								//serialize
								memcpy(&sendBuff[0], &masterFileList[0], numOfFiles*sizeof(struct listEntry));
								
								//send to all other clients except listener and ourselves
								int j;
								for(j = 0; j <= fdmax; j++){
									if(FD_ISSET(j, &master)){
										if(j != listener){
											nbytes = send(j, sendBuff, numOfFiles*sizeof(struct listEntry), 0);
										}
									}
								}
								
							}else{
								//reject client
								printf("Client %s already exists, rejecting!\n", temp[0].clientName);
								response = 'n';
								
								//send reject response to client
								memcpy(&sendBuff, &response, 1);
								if(send(i, sendBuff, 1, 0) < 1){
									printf("error on send!\n");
								}
								
								close(i);
								FD_CLR(i, &master);
							}
							

							
						}else if(command == 'b'){
							printf("Server: Received LS packet!\n");
							
							//send master list to client
							memcpy(&sendBuff[0], &masterFileList[0], numOfFiles*sizeof(struct listEntry));
							nbytes = send(i, sendBuff, numOfFiles*sizeof(struct listEntry), 0);
							
						}else if(command == 'c'){
							printf("Server: Received exit packet. Disconnecting client %s, and sending new list to remaining clients.\n", temp[0].clientName);
							//disconnect from client
							close(i);
							FD_CLR(i, &master);
							
							//remove their files from master list
							numOfFiles = removeFiles(temp[0].clientName, masterFileList, numOfFiles);
							
							//send master list to clients
							//serialize
							memcpy(&sendBuff[0], &masterFileList[0], numOfFiles*sizeof(struct listEntry));
								
							//send to all other clients except listener and ourselves
							int j;
							for(j = 0; j <= fdmax; j++){
								if(FD_ISSET(j, &master)){
									if(j != listener){
										nbytes = send(j, sendBuff, numOfFiles*sizeof(struct listEntry), 0);
									}
								}
							}
							
						}else {
							printf("Bad packet? =< \n");
						}
						
					}
				}//End Handle data from client
			}// End FD_ISSET()
		}//End connections loop 
	}//End Main Loop	
 } //End Main

