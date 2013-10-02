#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include "header.h"

#define MAXBUFSIZE 256
#define MAXMSGSIZE 50000

/* You will have to modify the program below */
void greeting();

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes received and sent by sendto()
	int toSend;								//size of bytes to be sent by sendto()
	int sockfd;                             //this will be our socket
	char buffer[MAXMSGSIZE];				//Buffer to receive respones from server
	char msg[MAXMSGSIZE];					//Message or command to be sent to the server
	int done = 0;							//bool for control looping
	struct sockaddr_in remote;             //"Internet socket address structure"
	int fileError = 0; 

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	 
	bzero(&remote,sizeof(remote));               //A brief explanation of some of the functions used in the code is provided here. However, for in depth understanding of the functions, please read the ma//zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	sockfd = socket(PF_INET,SOCK_DGRAM,0);
	if (sockfd < 0)
	{
		printf("unable to create socket");
	}
	
	//Tell user possible commands
	 greeting();
	 while(!done){
		int BUFFERSIZE = 256;
		char input[BUFFERSIZE]; //buffer to standard input
		fgets(input, BUFFERSIZE, stdin); 
		
		char command[BUFFERSIZE];
		char file[BUFFERSIZE];
		int n;

		
		//Parse through command
		n = sscanf(input, "%s %s", command, file);
		
		//Get [file_name] command
		if(strcmp(command, "get") == 0 && n == 2){
			bzero(&msg,sizeof(msg));
			strcpy(msg, input);
			toSend = strlen(msg);
		
		//Put [file_name] command	
		}else if(strcmp(command, "put") == 0 && n == 2){ 
			FILE *fp; 
			long file_size;
			
			//open file
			fp=fopen(file, "rb"); 
		
			if(fp == NULL){
				printf("Error opening file!\n");
				continue;
			}
		
			//obtain file size
			fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);
			
			bzero(&msg,sizeof(msg));
			size_t result = fread(msg, 1, file_size, fp);
			if(result != file_size){
				printf("File reading error!");
				fclose(fp);
				continue;
			}
			toSend = result;
			
			fclose(fp);
		
		//ls command	
		}else if(strcmp(command, "ls") == 0 && n == 1){
			bzero(&msg,sizeof(msg));
			strcpy(msg, "ls");
			toSend = strlen(msg);
				
		}else if(strcmp(command, "exit") == 0 && n == 1){//exit command
			printf("Telling server to bug off! \n");
			bzero(&msg,sizeof(msg));
			strcpy(msg, "exit");
			toSend = strlen(msg);
			done = 1;
		}else{ //non valid command
			printf("invalid command!(client)\n");
			continue; //continue so nothing is sent to server.
		}
		
	  /******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
		
		nbytes = sendto(sockfd, msg, toSend, 0,  (struct sockaddr *)&remote, sizeof(remote)); 
		//printf("Client sent: %d bytes", nbytes);
		if(nbytes < 0){
			printf("Client says: Error sending response to server\n");
		}

		// Blocks till bytes are received
		struct sockaddr_in from_addr;
		socklen_t addr_length = sizeof from_addr;
		bzero(buffer, sizeof(buffer));
		nbytes = recvfrom(sockfd, buffer, MAXMSGSIZE, 0, (struct sockaddr*)&from_addr, &addr_length);
		if(nbytes < 0){
			printf("Client says: Error reading server response\n");
		}

		printf("Server says %s\n\n", buffer);
		
		//SPECIAL CASE FOR PUT GIVE FILENAME TO SERVER ON REQUEST
		if(strcmp(buffer, "FilenameRequest")==0){
			//send filename
			printf("file on client side: %s \n", file);
			nbytes = sendto(sockfd, file, strlen(file), 0,  (struct sockaddr *)&remote, sizeof(remote));
			if(nbytes < 0){
				printf("Client says: Error reading server response\n");
			}
			
			//Receive success/fail
			nbytes = recvfrom(sockfd, buffer, MAXMSGSIZE, 0, (struct sockaddr*)&from_addr, &addr_length);
			if(nbytes < 0){
				printf("Client says: Error reading server response\n");
			}
			printf("Server says %s\n\n", buffer);
		}
		
		//If server returns file error on get, dont write to file
		if(strcmp(buffer, "Error opening file on server!")==0){
			fileError = 1;
		}else{
			fileError = 0;
		}
		
		//SPECIAL CASE FOR GET FILENAME
		if(strcmp(command,"get")==0 && !fileError){		
			//write file
			FILE * fp;
			fp = fopen(file, "wb");
			if(fwrite(buffer, 1, nbytes, fp) != nbytes || fp == NULL){
				printf("File write failure!\n");
			}else{
				printf("File write success!\n");
			}
			fclose(fp);	
		}

	 }
	close(sockfd);
	return 0; 
}

//display valid commands to user
void greeting(){
	
	printf("Please enter one of the following commands: \n");
	printf("get [file_name]   -   The server transmits the requested file to the client\n");
	printf("put [file_name]   -   The server receives the transmitted file by the client and stores it locally.\n");
	printf("ls                -   The server should search all the files it has in its local directory and send a list of all these files to the client.\n");
	printf("exit              -   The server should exit gracefully.\n\n");
	
}

