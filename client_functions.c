/* MAPS
	1 -> FileHash --verify
	2 -> FileHash --checkall
	3 -> FileDownload --TCP
	4 -> FileDownload --UDP
	5 -> IndexGet --shortlist
	6 -> IndexGet --longlist
	7 -> IndexGet --regex
	8 -> FileUpload --TCP
	9 -> FileUpload --UDP
*/

void client_fileHash(int sock, char *filename)
{
	char Command[BUFFER_SIZE] = "1 ";
	strcat(Command, filename);
	strcat(Command, " -");

	send(sock, Command, strlen(Command), 0);

	printf("%s: ", filename);
	printData(sock);

}

void client_fileHashAll(int sock)
{
	char Command[BUFFER_SIZE] = "2 - -";

	send(sock, Command, strlen(Command), 0);

	printData(sock);
}

void client_download_tcp(int sock, char *filename)
{
	char Command[BUFFER_SIZE] = "3 ";
	strcat(Command, filename);
	strcat(Command, " -");

	send(sock, Command, strlen(Command), 0);

	FILE *file = fopen(filename, "wb");
	putData(sock, file);
	fclose(file);

	FILE *fp = fopen(filename, "r");	
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fclose(fp);

	printf("%d(bytes)\t", sz);

	client_fileHash(sock, filename);

	getMD5(filename, Command);
	printf("Local File Hash: %s\n", Command);
}

void client_download_udp(int sock, char *filename)
{
	int udpport = 10001;
	sockaddr_in serverAddr;
	char buffer[BUFFER_SIZE];
	
	int udpSock;
	if((udpSock = (socket(AF_INET, SOCK_DGRAM, 0))) < 0)
		fprintf(stderr, "ERROR could not create UDP socket\n");

	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(udpport);
	serverAddr.sin_addr.s_addr = inet_addr(IP);

	if(bind(udpSock, (sockaddr*)&serverAddr, sizeof(serverAddr))<0)
		fprintf(stderr, "ERROR could not bind UDP socket\n");


	char Command[BUFFER_SIZE] = "4 ";
	strcat(Command, filename);
	strcat(Command, " -");

	send(sock, Command, strlen(Command), 0);

	FILE *file = fopen(filename, "wb");
	putData_udp(udpSock, file);
	fclose(file);

	FILE *fp = fopen(filename, "r");	
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fclose(fp);

	close(udpSock);

	printf("%d(bytes)\t", sz);

	client_fileHash(sock, filename);

	getMD5(filename, Command);
	printf("Local File Hash: %s\n", Command);
}

void client_shortList(int sock, char *startTime, char *endTime)
{
	char Command[BUFFER_SIZE] = "5 ";
	strcat(Command, startTime);
	strcat(Command, " ");
	strcat(Command, endTime);
	
	send(sock, Command, strlen(Command), 0);
	
	printData(sock);
}

void client_longList(int sock)
{
	char Command[BUFFER_SIZE] = "6 - -";
	
	send(sock, Command, strlen(Command), 0);
	
	printData(sock);
}

void client_regex(int sock, char *expression)
{
	char Command[BUFFER_SIZE] = "7 ";
	strcat(Command, expression);
	strcat(Command, " -");
	
	send(sock, Command, strlen(Command), 0);
	
	printData(sock);
}

void client_upload_tcp(int sock, char *filename)
{
	char Command[BUFFER_SIZE] = "8 ";
	strcat(Command, filename);
	strcat(Command, " -");
	
	send(sock, Command, strlen(Command), 0);
	
	char buffer[BUFFER_SIZE];
	
	int bytes;
	if((bytes = recv(sock, buffer, BUFFER_SIZE-1, 0)) >= 0)
	{
		buffer[bytes] = 0;
		if(strcmp(buffer, "FileUploadDeny")==0)
			return;
		FILE *fp = fopen(filename, "rb");
		if(fp == NULL)
		{
			strcpy(buffer, "ERROR No such file");
			send(sock, buffer, strlen(buffer), 0);
		}
		else
		{
			while((bytes = fread(buffer, 1, BUFFER_SIZE - 1, fp)) > 0)
			{
				buffer[bytes] = 0;
				send(sock, buffer, bytes, 0);
				if(bytes < BUFFER_SIZE - 1)
					break;
			}
		}
	}
}

void client_upload_udp(int sock, char *filename)
{
	char Command[BUFFER_SIZE] = "9 ";
	strcat(Command, filename);
	strcat(Command, " -");
	
	send(sock, Command, strlen(Command), 0);
	
	char buffer[BUFFER_SIZE];
	
	int bytes;
	if((bytes = recv(sock, buffer, BUFFER_SIZE-1, 0)) >= 0)
	{
		buffer[bytes] = 0;
		printf("[%d] %s\n", bytes, buffer);
		buffer[bytes] = 0;
		if(strcmp(buffer, "FileUploadDeny")==0)
			return;
			
		int udpport = 10001;
		int udpSock;
		sockaddr_in serverAddr;

		if((udpSock = (socket(AF_INET, SOCK_DGRAM, 0))) < 0)
			fprintf(stderr, "ERROR could not create UDP socket\n");

		bzero(&serverAddr, sizeof(serverAddr));

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(udpport);
		serverAddr.sin_addr.s_addr = inet_addr(IP);

		FILE *fp = fopen(filename, "rb");
		if(fp == NULL)
		{
			strcpy(buffer, "ERROR No such file");
			sendto(udpSock, buffer, strlen(buffer), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		}
		else
		{
			int bytes = 0;
			while((bytes = fread(buffer, 1, BUFFER_SIZE - 1, fp)) > 0)
			{
				buffer[bytes] = 0;
				sendto(udpSock, buffer, bytes, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
				if(bytes < BUFFER_SIZE - 1)
					break;
			}
		}

		close(udpSock);
	}
}
