/*
 * Computer Networks Assignment 2
 * An Application Level File­Sharing­Protocol with support for download and upload for files and indexed searching.
 * By: Saksham Aggarwal - 201301111
 *     Tanmay Sahay     - 201301173
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

#define BUFFER_SIZE 1024

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef unsigned char uchar;

typedef struct file {
	char name[1024];
	char time[1024];
	char type[100];
	int size;
}fileStruct;

fileStruct serverFiles[1024];

int clientPort = 5006;
int serverPort = 5005;
int TCP;
char commands[10][BUFFER_SIZE];
int commands_len;
int fileCount = 0;
char IP[30];

#include "helpers.h"
#include "client_functions.c"
#include "server_functions.c"

void startServer(int portno)
{
	printf("SERVER\n");

	int listenSocket = 0;
	int connectionSocket = 0;

	sockaddr_in serv_addr;

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if(listenSocket<0)
	{
		printf("ERROR while creating socket\n");
		return;
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portno);

	if(bind(listenSocket,(sockaddr * )&serv_addr,sizeof(serv_addr))<0)
		printf("ERROR while binding the socket\n");
	
	if(listen(listenSocket, 10) < 0)
		printf("ERROR failed to extablish connection\n");
	
	while((connectionSocket=accept(listenSocket , (sockaddr*)NULL,NULL))<0);

	char buffer[BUFFER_SIZE];

	while(1)
	{
		int len = recv(connectionSocket, buffer, BUFFER_SIZE-1, 0);
		if(len <= 0)
		{
			continue;
		}

		buffer[len] = 0;

		printf("[SERVER] [%d]: %s\n", len, buffer);

		if(strcmp(buffer, "quit")==0)
			break;

		int command;
		char filename[BUFFER_SIZE];
		char cmd2[BUFFER_SIZE];
		sscanf(buffer, "%d %s %s", &command, filename, cmd2);

		if(command == 1)
			server_fileHash(connectionSocket, filename);
		else if(command == 2)
			server_fileHashAll(connectionSocket);
		else if(command == 3)
			server_download_tcp(connectionSocket, filename);
		else if(command == 4)
			server_download_udp(filename);
		else if(command == 5)
			server_shortList(connectionSocket, filename, cmd2);
		else if(command == 6)
			server_longList(connectionSocket);
		else if(command == 7)
			server_regex(connectionSocket, filename);
		else if(command == 8)
			server_upload_tcp(connectionSocket, filename);
		else if(command == 9)
			server_upload_udp(connectionSocket, filename);

	}
	close(connectionSocket);
	close(listenSocket);
}

void startClient(int portno)
{
	printf("CLIENT\n");

	int clientSocket = 0;
	sockaddr_in serv_addr;

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if(clientSocket<0)
	{
		printf("ERROR while creating socket\n");
		return;
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = inet_addr(IP);

	while(connect(clientSocket,(sockaddr *)&serv_addr, sizeof(serv_addr))<0);

	char buffer[BUFFER_SIZE];

	while(1)
	{
		getchar();
		scanf("%[^\n]", buffer);
		
		if(strcmp(buffer, "quit")==0)
		{
			send(clientSocket, buffer, strlen(buffer), 0);
			break;
		}

		commands_len = 0;
		char *ptr;
		for(ptr = strtok(buffer, " \n"); ptr!=NULL; ptr = strtok(NULL, " \n"))
			strcpy(commands[commands_len++], ptr);

		if(strcmp(commands[0], "FileHash")==0)
		{
			if(strcmp(commands[1], "--verify") == 0)
				client_fileHash(clientSocket, commands[2]);
			else if(strcmp(commands[1], "--checkall") == 0)
				client_fileHashAll(clientSocket);
		}
		else if(strcmp(commands[0], "FileDownload")==0)
		{
			if(strcmp(commands[1], "--TCP") == 0)
				client_download_tcp(clientSocket, commands[2]);
			else if(strcmp(commands[1], "--UDP") == 0)
				client_download_udp(clientSocket, commands[2]);
		}
		else if(strcmp(commands[0], "IndexGet")==0)
		{
			if(strcmp(commands[1], "--shortlist") == 0)
				client_shortList(clientSocket, commands[2], commands[3]);
			else if(strcmp(commands[1], "--longlist") == 0)
				client_longList(clientSocket);
			else if(strcmp(commands[1], "--regex") == 0)
				client_regex(clientSocket, commands[2]);
		}
		else if(strcmp(commands[0], "FileUpload")==0)
		{
			if(strcmp(commands[1], "--TCP") == 0)
				client_upload_tcp(clientSocket, commands[2]);
			else if(strcmp(commands[1], "--UDP") == 0)
				client_upload_udp(clientSocket, commands[2]);
		}

		printf("Processed\n");
		
	}
	close(clientSocket);
}

int main()
{
	printf("Enter Local Port: ");
	scanf("%d", &serverPort);
	printf("Enter Remote IP: ");
	scanf("%s", IP);
	printf("Enter Remote Port: ");
	scanf("%d",&clientPort);
	
	int pid = fork();
	if(pid)
		startServer(serverPort);
	else
		startClient(clientPort);
	
	kill(pid, SIGKILL);
	
	return 0;
}
