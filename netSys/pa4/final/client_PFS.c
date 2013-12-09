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
#include <dirent.h>
#include <sys/stat.h>

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

//checks if file is on file list returns its index on list.
int checkFile(char client[], char file[], struct listEntry masterFileList[], int masterListSize){
	int i;
	for(i = 0; i < masterListSize; i++){
		if(strcmp(masterFileList[i].filename, file) == 0){ //found matching file
			if(strcmp(masterFileList[i].clientName, client) != 0){ //not from list this client
				return i;
			}
		}
	}
	return -1;
}

//prints file list
void printList(struct listEntry fileList[], int size){
	int i;
	for(i = 0; i < size; i++){
		printf("file: %s; size: %d; client: %s; ip: %s; port: %d\n", fileList[i].filename, 
			fileList[i].filesize, fileList[i].clientName, fileList[i].clientIp, fileList[i].port);
	}
}

//prints file list
void printFiles(struct listEntry fileList[], int size){
	int i;
	for(i = 0; i < size; i++){
		printf("%-16s : %d KB\n", fileList[i].filename, fileList[i].filesize/1024);
	}
}

//returns # of files populated
int fillList(char command, char clientName[], char clientIp[], int port, struct listEntry fileList[]){
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	int rows = 0;
	
	if ((dir = opendir ("./")) != NULL) {
	/* for each file*/
		while ((ent = readdir (dir)) != NULL) {
			lstat(ent->d_name, &statbuf);
			if(!S_ISDIR(statbuf.st_mode)){
				//printf ("%s size: %d\n", ent->d_name, (int)statbuf.st_size);
				fileList[rows].command = command;
				strcpy(fileList[rows].filename, ent->d_name);
				fileList[rows].filesize = (int)statbuf.st_size;
				strcpy(fileList[rows].clientName, clientName);
				strcpy(fileList[rows].clientIp, clientIp);
				fileList[rows].port = port;
				rows++;
			}	
		}
	closedir (dir);
	} else {
	/* could not open directory */
		perror ("Could not read local directory!\n");
		return EXIT_FAILURE;
	}	
	
	return rows;
}

 int main(int argc, const char* argv[]){
	 
	int serverSock, getSock;
	int nbytes, listSize, packetSize, maxfd, masterListSize;
	struct sockaddr_in myAddr, servAddr;
	socklen_t myAddrLen;
	struct listEntry fileList[MAXFILES];
	struct listEntry masterFileList[MAXFILES];
	char* sendBuffer[MAXBUFSIZE];
	char* recvBuffer[MAXBUFSIZE];
	char command;
	
	//check command line arguments
	if(argc !=4){
		printf("Usage: ./client_PFS <Client Name> <Server IP> <Server Port>\n");
		return -1;
	}
	 
	//Open socket to connect to file server
	serverSock=socket(AF_INET,SOCK_STREAM,0);
	if(serverSock < 0){
		printf("Error creating server socket!\n");
		return -1;
	}
	
	//populate server info
	bzero(&servAddr,sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=inet_addr(argv[2]);
	servAddr.sin_port=htons(atoi(argv[3]));
	
	//connect to file server
	if(connect(serverSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0){
		printf("Error connecting to file server!\n");
		return -1;
	}

	//open socket for get requests
	getSock = socket(AF_INET, SOCK_STREAM,0);
	if(getSock < 0){
		printf("Error creating get socket!\n");
		return -1;
	}
	
	//populate local info
	bzero(&myAddr,sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr=INADDR_ANY; 
	myAddr.sin_port=htons(0);
	
	//bind to any open port
	if(bind(getSock, (struct sockaddr*)&myAddr, sizeof(myAddr))<0){
		printf("Error binding get socket!\n");
		return -1;
	}
	
	//get port and ip for get requests
	myAddrLen = sizeof(myAddr);
	if(getsockname(getSock, (struct sockaddr*)&myAddr, &myAddrLen)<0){
		printf("Error on getsockname()!\n");
		return -1;
	}
	
	if(listen(getSock, MAXCLIENTS) < 0){
		printf("error on getSock listen()\n");
		return -1;
	}
	
	printf("Client %s is waiting on %s:%d\n", argv[1], inet_ntoa(myAddr.sin_addr), ntohs(myAddr.sin_port));
	
	//send connect packet.
	command = 'a';
	bzero(&sendBuffer, sizeof(sendBuffer));
	
	//store local files into a buffer
	listSize = fillList(command, argv[1], inet_ntoa(myAddr.sin_addr), ntohs(myAddr.sin_port), fileList);
	packetSize = (listSize * sizeof(struct listEntry));

	if(listSize == 0){
		fileList[0].command = 'a';
		strcpy(fileList[0].clientName, argv[1]);
		strcpy(fileList[0].clientIp, inet_ntoa(myAddr.sin_addr));
		fileList[0].port = ntohs(myAddr.sin_port);
		listSize = 1;
		packetSize = (listSize * sizeof(struct listEntry));
	}else if(listSize >= MAXFILES){
		printf("Too many files in directory!\n");
		return -1;
	}
	
	//serialize packet
	memcpy(&sendBuffer[0], &fileList[0], packetSize);
	nbytes = send(serverSock, sendBuffer, packetSize, 0);
	if(nbytes < packetSize){
		printf("Error sending connect packet!\n");
		return -1;
	}
	
	//wait for user input, get request, or response from server
	
	fd_set master;    // master file descriptor list
    fd_set readfds;  // temp file descriptor list for select() 
	maxfd = 0;
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&readfds);
	
	FD_SET(0, &master); //stdin
	FD_SET(serverSock, &master); //server
	FD_SET(getSock, &master); //other clients
	if(serverSock > maxfd){ maxfd = serverSock; }
	if(getSock > maxfd){ maxfd = getSock; }
	
	printf("Client initalization completed with no errors!\n");
	printf("Enter one of the following commands: \n");
	printf("1) 'get <filename>' - Downloads file <filename> from a client\n");
	printf("2) 'ls'             - Prints the master file list\n");
	printf("3) 'exit'           - Shuts down client safely\n");
	
	for(;;){
		readfds = master;
		if (select(maxfd+1,&readfds,NULL,NULL,NULL) < 0){
			printf("select error!\n");
			return -1;
		}
		
		//got data from server
		if (FD_ISSET(serverSock, &readfds)){
			bzero(&recvBuffer, sizeof(recvBuffer));
			nbytes = recv(serverSock, recvBuffer, sizeof(recvBuffer), 0);
			printf("\n");
			if(nbytes == 0){
				printf("Lost connection to server =<\n");
				return -1;
			}
			if(nbytes < 0){
				printf("error on recv()\n");
				continue;
			}
			if(nbytes == 1){
				printf("Rejected by server, client name already exists!\n");
				return -1;
			}
			
			masterListSize = (nbytes)/sizeof(struct listEntry);
			memcpy(&masterFileList[0], &recvBuffer[0], nbytes);
			printf("File List: \n");
			printFiles(masterFileList, masterListSize);
		}
		//got data from client
		else if (FD_ISSET(getSock, &readfds)){			
			//populate client to send file to.
			struct sockaddr_in sendAddr;
				
			bzero(&sendAddr, sizeof(sendAddr));
			//sendAddr.sin_family=AF_INET;
			//sendAddr.sin_addr.s_addr=inet_addr(masterFileList[index].clientIp);
			//sendAddr.sin_port=htons(masterFileList[index].port);
			
			socklen_t addrlen = sizeof sendAddr;
			int newfd = accept(getSock, (struct sockaddr *)&sendAddr, &addrlen);
			if(newfd < 0){
				printf("Error on accept!\n");
				continue;
			}
			
			//recv filename to send
			char getFile[16];
			nbytes = recv(newfd, getFile, sizeof(getFile), 0);
			if(nbytes < 0){
				printf("error on recv!\n");
				continue;
			}
			
			printf("Client has requested file: %s\n", getFile);
			
			//open and read file requested
			FILE* fp;
			fp = fopen (getFile, "rb");
			if(fp == NULL){
				printf("error opening file to send()\n");
			}
			
			//obtain file size
			fseek(fp, 0, SEEK_END);
			long file_size = ftell(fp);
			rewind(fp);
			
			bzero(&sendBuffer,sizeof(sendBuffer));
			size_t result = fread(sendBuffer, 1, file_size, fp);
			if(result != file_size){
				printf("File reading error!");
				fclose(fp);
			}
			
			//send file
			nbytes = send(newfd, sendBuffer, result, 0);
			
			//close connection
			fclose(fp);
			close(newfd);
			
			printf("File successfully sent to client!\n");
		}
		//got data from STDIN
		else if (FD_ISSET(0, &readfds)){
			char stdinBuffer[32];
			fgets(stdinBuffer, 32, stdin);
			
			char file[16];
			char command[16];
			bzero(&file, sizeof(file));
			bzero(&command, sizeof(command));
			sscanf(stdinBuffer, "%s %s", command, file);
			
			if(strcmp("ls\n", stdinBuffer) == 0){
				bzero(&sendBuffer, sizeof(sendBuffer));
				struct listEntry temp;
				temp.command = 'b';
				memcpy(&sendBuffer[0], &temp, sizeof(temp));
				nbytes = send(serverSock, sendBuffer, sizeof(temp), 0);
				
			}else if(strcmp("exit\n", stdinBuffer) == 0){
				printf("Cya!\n");
				bzero(&sendBuffer, sizeof(sendBuffer));
				struct listEntry temp;
				temp.command = 'c';
				strcpy(temp.clientName, argv[1]);
				memcpy(&sendBuffer[0], &temp, sizeof(temp));
				nbytes = send(serverSock, sendBuffer, sizeof(temp), 0);
				return 0;
			
			}else if(strcmp("get", command) == 0){
				printf("Getting file: %s\n", file);
				
				//check if file is on master file list
				//also make sure it is not your own file
				int index = checkFile(argv[1], file, masterFileList, masterListSize);
				if(index == -1){
					printf("File does not exist or belongs to this client.\n");
					continue;
				}
				
				//connect to client that has file
				//Open socket to connect to file server
				
				int clientSock=socket(AF_INET,SOCK_STREAM,0);
				if(clientSock < 0){
					printf("Error creating client socket!\n");
					continue;
				}
				
				//populate client to connect to info.
				struct sockaddr_in clientAddr;
				
				bzero(&clientAddr, sizeof(clientAddr));
				bzero(&recvBuffer, sizeof(recvBuffer));
				clientAddr.sin_family=AF_INET;
				clientAddr.sin_addr.s_addr=inet_addr(masterFileList[index].clientIp);
				clientAddr.sin_port=htons(masterFileList[index].port);
	
				//connect to client
				if(connect(clientSock, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0){
					printf("Could not connect to client!\n");
					continue;
				}
				
				//request file
				nbytes = send(clientSock, file, sizeof(file), 0);
				if(nbytes < 0){
					printf("error on send()!\n");
					continue;
				}
				
				//receive file
				nbytes = recv(clientSock, recvBuffer, sizeof(recvBuffer), 0);
				if(nbytes < 0){
					printf("error on recv!\n");
				}
				
				//write file to drive
				FILE * fp;
				fp = fopen(file, "wb");
				
				if(fwrite(recvBuffer, 1, nbytes, fp)!= nbytes || fp == NULL){
					printf("error writing file!\n");
				}
				
				fclose(fp);
				printf("Succesfully got file!\n");
			}else{
				printf("Invalid Command!\n");
			}
		}else{
			printf("uhh?!\n");
		}
	}
}
