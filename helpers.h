void getMD5(char *filename, char *hash)
{
	FILE *inFile = fopen(filename, "r");
	if(inFile == NULL)
	{
		hash[0] = 0;
		return;
	}

	MD5_CTX mdContext;
	char buffer[BUFFER_SIZE];
	uchar c[20];
	int i, bytes;

	MD5_Init (&mdContext);
    while ((bytes = fread (buffer, 1, BUFFER_SIZE-1, inFile)) != 0)
        MD5_Update (&mdContext, buffer, bytes);
    MD5_Final (c, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH; i++)
    	sprintf(hash+2*i, "%02x", c[i]);

    hash[2*i] = 0;
    
    fclose (inFile);
}

void getLastModified(char *filename, char *str)
{
	struct stat attr;
	stat(filename, &attr);
	sprintf(str, "%s", ctime(&attr.st_mtime));
}

void printData(int sock)
{
	char buffer[BUFFER_SIZE];

	int bytes;
	while((bytes = recv(sock, buffer, BUFFER_SIZE-1, 0)) > 0)
	{
		buffer[bytes] = 0;
		printf("%s", buffer);
		if(bytes < BUFFER_SIZE-1)
			break;
	}

	printf("\n");
}

void putData(int sock, FILE *fp)
{
	char buffer[BUFFER_SIZE];

	int bytes;
	int sum = 0;
	while((bytes = recv(sock, buffer, BUFFER_SIZE-1, 0)) >= 0)
	{
		buffer[bytes] = 0;
		fwrite(buffer, 1, bytes, fp);
		if(bytes < BUFFER_SIZE-1)
			break;
		sum += bytes;
		if(sum > 1e5)
		{
			fprintf(stderr, "~");
			sum -= 1e5;
		}
	}
	fprintf(stderr, "\n");
}

void putData_udp(int sock, FILE *fp)
{
	sockaddr_in clientAddr;

	char buffer[BUFFER_SIZE];

	int bytes;
	int sum = 0;
	int tot = 0;
	int len = sizeof(clientAddr);
	while((bytes = recvfrom(sock, buffer, BUFFER_SIZE-1, 0, (sockaddr*)&clientAddr, &len)) > 0)
	{
		buffer[bytes] = 0;
		fwrite(buffer, 1, bytes, fp);
		if(bytes < BUFFER_SIZE-1)
			break;
		sum += bytes;
		tot += bytes;
		if(sum > 1e5)
		{
			fprintf(stderr, "~");
			sum -= 1e5;
		}
	}
	fprintf(stderr, "\n");
}

int splread(FILE *fp, char *buffer, int size)
{
	int i;
	char c;
	for(i=0; i<size; i++)
	{
		if(fscanf(fp, "%c", buffer+i) == EOF)
			break;
	}
	buffer[i] = 0;
	printf("---------%d----\n", i);
	return i;
}

int min(int a, int b)
{
	return a<b?a:b;
}

void updateFileList()
{
	DIR* directory;
	directory = opendir("./");
	if (directory)
	{
		struct dirent *temp;
		int i;
		for (i = 0; temp = readdir(directory); i++)
		{
			// name of the file
			char *tmp = temp->d_name;
			if(tmp[0] == '.')
			{
				i--;
				continue;
			}

			strcpy(serverFiles[i].name, tmp);
			
			// details of file
			struct stat details;
			stat(temp->d_name, &details);
			
			// size of the file
			serverFiles[i].size = details.st_size;
			
			// timestamp of the file
			strcpy(serverFiles[i].time, ctime(&details.st_mtime));
			serverFiles[i].time[strlen(serverFiles[i].time) - 1] = 0;
			
			// type of the file
			// reference: http://man7.org/linux/man-pages/man2/stat.2.html
			if ((details.st_mode & S_IFMT) == S_IFREG)
				strcpy(serverFiles[i].type, "Regular File");
			else if ((details.st_mode & S_IFMT) == S_IFSOCK)
				strcpy(serverFiles[i].type, "Socket");
			else if ((details.st_mode & S_IFMT) == S_IFLNK)
				strcpy(serverFiles[i].type, "Symbolic Link");
			else if ((details.st_mode & S_IFMT) == S_IFBLK)
				strcpy(serverFiles[i].type, "Block Device");
			else if ((details.st_mode & S_IFMT) == S_IFDIR)
				strcpy(serverFiles[i].type, "Directory");
			else if ((details.st_mode & S_IFMT) == S_IFCHR)
				strcpy(serverFiles[i].type, "Character Device");
			else if ((details.st_mode & S_IFMT) == S_IFIFO)
				strcpy(serverFiles[i].type, "FIFO");
		}
		fileCount = i - 1;
		closedir(directory);
	}
}
