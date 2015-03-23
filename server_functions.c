void server_fileHash(int sock, char *filename)
{
	char hash[BUFFER_SIZE] = "";
	getMD5(filename, hash);
	
	if(hash[0] == 0)
		strcpy(hash, "ERROR: file not found!");
	else
	{
		strcat(hash, "\t");
		getLastModified(filename, hash+33);
	}
	send(sock, hash, strlen(hash), 0);
}

void server_fileHashAll(int sock)
{
	char buffer[BUFFER_SIZE*500];
	char hash[BUFFER_SIZE];
		
	DIR *d;
	struct dirent *dir;

	d = opendir(".");
	while((dir = readdir(d)) != NULL)
	{
		char *filename = dir->d_name;
		
		if(filename[0] == '.')
			continue;

		hash[0] = 0;
		
		strcat(hash, filename);
		strcat(hash, ":\t\t");

		getMD5(filename, hash+strlen(hash));

		strcat(hash, "\t\t");
		getLastModified(filename, hash+strlen(hash));
		
		strcat(buffer, hash);
	}
	closedir(d);

	char *start = buffer;
	int len;
	for(len = strlen(buffer); len > 0; len -= BUFFER_SIZE-1, start += BUFFER_SIZE-1)
		send(sock, start, min(len, BUFFER_SIZE-1), 0);

}

void server_download_tcp(int sock, char *filename)
{
	char buffer[BUFFER_SIZE];
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL)
	{
		strcpy(buffer, "ERROR No such file");
		send(sock, buffer, strlen(buffer), 0);
	}
	else
	{
		int bytes = 0;
		while((bytes = fread(buffer, 1, BUFFER_SIZE - 1, fp)) > 0)
		{
			buffer[bytes] = 0;
			send(sock, buffer, bytes, 0);
			if(bytes < BUFFER_SIZE - 1)
				break;
		}
	}
}

void server_download_udp(char *filename)
{
	int udpport = 10001;
	int udpSock;
	sockaddr_in serverAddr;

	if((udpSock = (socket(AF_INET, SOCK_DGRAM, 0))) < 0)
		fprintf(stderr, "ERROR could not create UDP socket\n");

	bzero(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(udpport);
	serverAddr.sin_addr.s_addr = inet_addr(IP);

	char buffer[BUFFER_SIZE];
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

time_t convert(char *tempTime)
{
	struct tm tm;
	strptime(tempTime, "%a %b %d  %H:%M:%S %Y\name", &tm);
	time_t ret = mktime(&tm);
	return ret;
}

time_t humanConvert(char *tempTime)
{
	struct tm tm;
	strptime(tempTime, "%Y-%m-%d-%H-%M-%S", &tm);
	time_t ret = mktime(&tm);
	return ret;
}

void server_shortList(int sock, char *startTime, char *endTime)
{
	updateFileList();
	
	time_t beginTime = humanConvert(startTime);
	time_t finishTime = humanConvert(endTime);
	
	char buffer[BUFFER_SIZE*100] = "";
	int i;
	for (i = 0; i <= fileCount; i++)
	{
		time_t timestamp = convert(serverFiles[i].time);
		if (difftime(beginTime, timestamp) < 0 && difftime(timestamp, finishTime) < 0)
		{
			char tmp[BUFFER_SIZE];
			sprintf(tmp, "%s\t%s\t%s\t%d\n", serverFiles[i].name, serverFiles[i].time, serverFiles[i].type, serverFiles[i].size);
			strcat(buffer, tmp);
		}
	}
	send(sock, buffer, strlen(buffer), 0);
}

void server_longList(int sock)
{
	updateFileList();
	
	char buffer[BUFFER_SIZE*100] = "";
	int i;
	for (i = 0; i <= fileCount; i++)
	{
		char tmp[BUFFER_SIZE];
		sprintf(tmp, "%s\t%s\t%s\t%d\n", serverFiles[i].name, serverFiles[i].time, serverFiles[i].type, serverFiles[i].size);
		strcat(buffer, tmp);
	}
	send(sock, buffer, strlen(buffer), 0);
}

void server_regex(int sock, char *expression)
{
	updateFileList();
	
	if(expression[0] == '"')
	{
		expression++;
		expression[strlen(expression) - 1] = 0;
	}

	char buffer[BUFFER_SIZE*100] = "";
	// reference: http://www-01.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rtref/regexec.htm%23regexec
	regex_t preg;
	int rc = regcomp(&preg, expression, 0);
	if (rc)
	{
		fprintf(stderr, "Invalid RegEx\n");
		return;
	}
	int i;
	size_t nmatch = 1;
	regmatch_t pmatch[2];
	for (i = 0; i <= fileCount; i++)
	{
		if (!regexec(&preg, serverFiles[i].name, nmatch, pmatch, 0))
		{
			char tmp[BUFFER_SIZE];
			sprintf(tmp, "%s\t%s\t%s\t%d\n", serverFiles[i].name, serverFiles[i].time, serverFiles[i].type, serverFiles[i].size);
			strcat(buffer, tmp);
		}
	}
	send(sock, buffer, strlen(buffer), 0);
}

void server_upload_tcp(int sock, char *filename)
{
	char response = 'Y';
	if (response == 'Y' || response == 'y')
	{
		send(sock, "FileUploadAllow", 15, 0);
		FILE *fp = fopen(filename, "wb");
		
		putData(sock, fp);
		
		fclose(fp);
		
		char buffer[BUFFER_SIZE];
		
		getMD5(filename, buffer);
		printf("%s:\t%s\t", filename, buffer);
		getLastModified(filename, buffer);
		printf("%s\n", buffer);
	}
	else
	{
		send(sock, "FileUploadDeny", 14, 0);
	}
}

void server_upload_udp(int sock, char *filename)
{
	char response = 'Y';
	if (response == 'Y' || response == 'y')
	{
		send(sock, "FileUploadAllow", 15, 0);
		
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

		FILE *fp = fopen(filename, "wb");
		
		putData_udp(udpSock, fp);
		
		fclose(fp);
		
		getMD5(filename, buffer);
		printf("%s:\t%s\t", filename, buffer);
		getLastModified(filename, buffer);
		printf("%s\n", buffer);	
		
		close(udpSock);
	}
	else
	{
		send(sock, "FileUploadDeny", 14, 0);
	}
}
