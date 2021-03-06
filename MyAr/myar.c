/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin
Extra Credit Attempts:
	1) -q supports multiple files on command line
	2) -x suppots multiple files on command line
	3) -v is functional

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

void appendFiles(int argc, char *argv[]);
void addWhitespace(char *str, int size);
void extract(int argc, char *argv[]);
void trimWhitespace(char* str);
void printTable(int argc, char *argv[]);
void addAllRecent(int argc, char *argv[]);
void printVerbose(int argc, char *argv[]);
void printPermTextFromOctal(mode_t mode);
void delete(int argc, char *artv[]);
char *useage = "Usage: ./myar key afile filename ...\nq\tquickly append named files to archive\nx\textract named files\nt\tprint a concise table of contents of the archive\nv\tprint a verbose table of contents of the archive\nd\tdelete named files from archive\nA\tquickly append \"ordinary\" files in the current directory that have been modified within the last two hours\n";

typedef struct _node{
	char filename[17];
	struct _node *next;
	struct _node *prev;
}node;

typedef struct _headerNode{
	char header[61];
	struct _headerNode *next;
	struct _headerNode *prev;
}hNode;

/*MARK: Begin Functions*/
int main(int argc, char *argv[]){

	if (argc < 2){
		printf("%s",useage);
		return 0;
	}

	char* flag = argv[1];
	if (strlen(flag) != 2){
		printf("Invalid flag\n%s",useage);
	}

	int flagVal = flag[1];	
	switch (flagVal){
		case 'q':
		appendFiles(argc, argv);
		break;
		case 'x':
		extract(argc, argv);
		break;
		case 't':
		printTable(argc, argv);
		break;
		case 'v':
		printVerbose(argc, argv);
		break;
		case 'd':
		printf("d flag not in use\n");
		break;
		case 'A':
		addAllRecent(argc, argv);
		break;
		default:
		printf("Invalid flag\n%s", useage);
	}
}

void appendFiles(int argc, char *argv[]){
	if (argc < 4){
		printf("Not enough arguments to append files\n%s", useage);
	}
	
	char* archiveName = argv[2];
	struct stat archStats;
	int archDesc;
	int exists = stat(archiveName,&archStats);
	if (exists == -1){	//add archive header if necessary
		archDesc = open(archiveName, O_CREAT | O_RDWR | O_TRUNC, 0666);
		char* archHeader = "!<arch>\n";
		write(archDesc, archHeader, 8);
	}else{
		archDesc = open(archiveName, O_APPEND | O_WRONLY, 0666);
		if (archDesc < 0){
			printf("Error opening archive %d\n", archDesc);
			return;
		}
	}

	//Add header + body for each file
	for (int i = 3; i < argc; i++){
		char* fileName = argv[i];
		struct stat fileStats;
		int exists = stat(fileName, &fileStats);
		if (exists == -1){
			printf("Invalid not found: %s\n", fileName);
			continue;
		}
		//First Build file header, then put it on the toWrite string
		int headerSize = 61; //60 chars + null char
		char header[headerSize];

		int nameLen = strlen(fileName);
		for (int i = 0; i < nameLen; i++){
			header[i] = fileName[i];
		}
		for (int i = nameLen; i < 16; ++i){
			header[i] = ' ';
		}
		header[16] = '\0';			

		char timestamp[13];
		int n = sprintf(timestamp, "%ld", fileStats.st_mtime);
		addWhitespace(timestamp, 13);
		strcat(header, timestamp);

		char ownerID[7];
		n = sprintf(ownerID, "%u", fileStats.st_uid);
		addWhitespace(ownerID, 7);
		strcat(header, ownerID);

		char groupID[7];
		n = sprintf(groupID, "%u", fileStats.st_gid);
		addWhitespace(groupID, 7);
		strcat(header, groupID);

		char mode[9];
		n = sprintf(mode, "%o", fileStats.st_mode);
		addWhitespace(mode, 9);
		strcat(header, mode);

		char fileSize[11];
		n = sprintf(fileSize, "%jd", fileStats.st_size);
		addWhitespace(fileSize, 11);
		strcat(header, fileSize);

		header[58] = '\x60';
		header[59] = '\x0A';
		header[60] = '\0';

		//Write Header
		if  (write(archDesc, header, 60) < 0){
			printf("error writing header: %d\n", errno);
		}

		//Write file contents in a loop		
		int fd = open(fileName, O_RDONLY);
		if (fd < 0){
			printf("Error getting file descriptor, errno %d\n", errno);
			continue;
		}
		size_t bytesToRead = fileStats.st_size;
		char content[fileStats.st_size];
		size_t bytesRead = 0;
		do{
			if ((n = read(fd, &((char *)content)[bytesRead], bytesToRead - bytesRead)) == -1){
				if (errno == EINTR){
					printf("encountered system interruption, continuing\n");
					continue;	//just an interruption, keep going
				}else{
					printf("error reading file: %d\n", errno);
					return;
				}
			}
			if (n == 0){	//nothing read on this call
				break;	//finished reading
			}
			bytesRead += n;
			//printf("bytes read: %d\n", n);
		}while(bytesToRead > bytesRead);
		//printf("total bytes read: %d\n", bytesRead);
		//printf("content: %s\n", content);
		int w = write(archDesc, content, bytesRead);
		if (w < 0){
			printf("error writing to file: errno: %d\n", errno);
		}
		if (w < bytesToRead){
			printf("Error, did not write entire file. %d", errno);
		}

		// if (fileStats.st_size % 2){
		// 	printf("writing odd char\n");
		// 	write(archDesc, '\n', 1);	//buffer to keep data 2 byte aligned
		// }
		close(fd);
	}
	close(archDesc);
}
void addWhitespace(char* str, int size){
	int len = strlen(str);
	for (int i = len; i < size; ++i){
		str[i] = ' ';
	}
	str[size - 1] = '\0';
}
void trimWhitespace(char* str){
	for (int i = 0; i < strlen(str); i++){
		if (str[i] == ' '){
			str[i] = '\0';
			return;
		}
	}
}
void extract(int argc, char *argv[]){
	//build linkedlist of filename
	if (argc < 4){
		printf("%s\n", useage);
		return;
	}
	node * head = NULL;
	head = malloc(sizeof(node));
	strcpy(head->filename, argv[3]);
	head->next = NULL;
	head->prev = NULL;
	addWhitespace(head->filename, 17);
	node * runner = head;
	for (int i = 4; i < argc; ++i){
		runner->next = malloc(sizeof(node));
		runner->next->prev = runner;
		runner = runner->next;
		strcpy(runner->filename, argv[i]);
		addWhitespace(runner->filename, 17);
		runner->next = NULL;
	}
	runner = head;
	while(runner != NULL){
		runner = runner->next;
	}

	//Run through archived file until all listed flies have been extracted
	char* archiveName = argv[2];
	struct stat archStats;
	int exists = stat(archiveName,&archStats);
	if (exists == -1){
		printf("Error, archive not found\n");
		return;
	}
	int archDesc = open(archiveName, O_RDONLY);
	if (lseek(archDesc, 8, SEEK_SET) < 0){
		printf("Error with archive file.\n");
		return;
	}
	while(head != NULL){ //continue until all files have been extracted
		char header[61];
		int n = read(archDesc, header, 60);
		if (n < 60) break;	//natural ending when end of archive is reached
		header[60] = '\0';
		//Check each filename in list
		runner = head;
		char* fileName;
		int match = 0;
		while(runner != NULL){
			fileName = runner->filename;
			match = 1;
			for (int i = 0; i < 16; ++i){	//check for name match in list
				if (header[i] != fileName[i]){	//compare character by character
					match = 0;
					runner = runner->next;
					break;
				}
			}
			if (match){	//if found a match, extract it
				break;
			}
		}

		//first get size of file. Need this for both reading and seeking
		char size[11];
		for (int i = 0; i < 10; ++i){
			size[i] = header[i+48];
		} size[10] = '\0';
		int contentSize = atoi(size);
		if (contentSize % 2 == 1){
			contentSize = contentSize + 1;
		}

		if (match == 0){	//no match for this header, go to next one;
			if (lseek(archDesc, contentSize, SEEK_CUR) < 0){
				printf("Error with archive file.\n");
				return;
			}
			continue;
		}

		trimWhitespace(fileName);	//legacy. not sure if safe to delete
		char timestamp[13];
		for (int i = 0; i < 12; ++i){
			timestamp[i] = header[i+16];
		} timestamp[12] = '\0';
		trimWhitespace(timestamp);
		struct utimbuf *timeBuff;
		timeBuff = malloc(sizeof(struct utimbuf));
		timeBuff->actime = atoi(timestamp);
		timeBuff->modtime = atoi(timestamp);
		char ownerID[7];
		for (int i = 0; i < 6; ++i){
			ownerID[i] = header[i+28];
		} ownerID[6] = '\0';
		uid_t ownerIDNum = atoi(ownerID);
		trimWhitespace(ownerID);
		char groupID[7];
		for (int i = 0; i < 6; ++i){
			groupID[i] = header[i+34];
		} groupID[6] = '\0';
		gid_t groupIDNum = atoi(groupID);
		trimWhitespace(groupID);
		char mode[9];
		for (int i = 0; i < 8; ++i){
			mode[i] = header[i+40];
		} mode[8] = '\0';
		trimWhitespace(mode);
		char perms[4];
		for (int i = 0; i < 4; ++i){
			perms[i] = mode[i+2];
		}

		int octalPerm;
		sscanf(perms, "%o", &octalPerm);
		int permVal = atoi(perms);
		int modeVal = atoi(mode);
		mode_t modeT = modeVal;


		//Load content into array from archive file
		char content[contentSize + 1];
		size_t bytesToRead = contentSize;
		size_t bytesRead = 0;
		do{
			if ((n = read(archDesc, &((char *)content)[bytesRead], bytesToRead - bytesRead)) == -1){
				if (errno == EINTR){
					printf("encountered system interruption, continuing\n");
					continue;	//just an interruption, keep going
				}else{
					printf("error reading file: %d\n", errno);
					return;
				}
			}
			if (n == 0){	//nothing read on this call
				break;	//finished reading
			}
			bytesRead += n;
			//printf("bytes read: %d\n", n);
		}while(bytesToRead > bytesRead);

		if (contentSize % 2 == 1){
			content[contentSize - 1] = '\0';
		}else{
			content[contentSize] = '\0';
		}

		//printf("filename: %s\n", fileName);
		int fd = open(fileName, O_TRUNC | O_CREAT | O_RDWR, octalPerm);
		if (fd < 0){
			printf("Error opening file. errno: %d\n", errno);
		}
		ssize_t bytesWritten = 0;
		do{
			if ((n = write(fd, &((char *)content)[bytesWritten], contentSize - bytesWritten)) == -1){
				if (errno == EINTR){
					continue;	//ignore system interruption
				}else{
					printf("Error writing file. Errno: %d \n", errno);
					return;
				}
			}
			bytesWritten += n;
		}while(bytesWritten < contentSize);

		
		chmod(fileName, octalPerm);
		chown(fileName, ownerIDNum, groupIDNum);
		utime(fileName, timeBuff);
		close(fd);
		if (runner == head){
			head = runner->next;
		}
		if (runner->prev != NULL){
			runner->prev->next = runner->next;
		}
		if (runner->next != NULL){
			runner->next->prev = runner->prev;
		}
	}
	return;
		
	//bring archive descriptor to first header
	
	//extracts all individual fils if no arguments given
	//extracts only named files if there are given names
}
void printTable(int argc, char *argv[]){

	//open archive and set offset to first header
	char* archiveName = argv[2];
	struct stat archStats;
	int exists = stat(archiveName,&archStats);
	if (exists == -1){
		printf("Error, archive not found\n");
		return;
	}
	int archDesc = open(archiveName, O_RDONLY);
	if (lseek(archDesc, 8, SEEK_SET) < 0){
		printf("Error with archive file.\n");
		return;
	}

	char header[61];	//hold header
	int n = read(archDesc, header, 60);
	header[60] = '\0';
	if (n < 60 && n > 0) {
		printf("Error in archive formatting\n");
		return;	//archive does not contain any headers
	}
	while (n > 0){
		//first get and print permissions
		
		char size[11];
		for (int i = 0; i < 10; ++i){
			size[i] = header[i+48];
		} size[10] = '\0';
		int contentSize = atoi(size);

		char fileName[17];
		fileName[16] = '\0';
		for (int i = 0; i < 16; ++i){
			fileName[i] = header[i];
		}
		trimWhitespace(fileName);
		printf("%s\n",fileName);

		//adjust for even text alignment
		if (contentSize % 2 == 1){
			contentSize = contentSize + 1;
		}
		if (lseek(archDesc, contentSize, SEEK_CUR) < 0){
			printf("Error with archive file.\n");
			return;
		}

		//read next header
		n = read(archDesc, header, 60);
		header[60] = '\0';
	}

}
void addAllRecent(int argc, char *argv[]){
	//open current directory
	DIR *directory;
	struct dirent *dir;
	directory = opendir(".");

	time_t now;
	now = time(0);
	int maxTimeDiff = 7200;
	if (directory){
		int numFiles = 0, n;
		struct stat dirStat;
		char mode[9];
		//first pass, create linkedlist of files and count them
		node * head = NULL;
		node * runner = head;

		while ((dir = readdir(directory)) != NULL){
			char *name = dir->d_name;
			stat(name, &dirStat);
			n = sprintf(mode, "%o", dirStat.st_mode);
			if (S_ISLNK(dirStat.st_mode)) continue;
			if (S_ISREG(dirStat.st_mode) && strcmp(argv[2], name)){
				int diff = now - dirStat.st_mtime;
				if(diff < maxTimeDiff){
					numFiles++;
					if (head == NULL){
						head = malloc(sizeof(node));
						head->next = NULL;
						head->prev = NULL;
						runner = head;
						strcpy(runner->filename, name);
					}else{
						runner->next = malloc(sizeof(node));
						runner->next->prev = runner;
						runner = runner->next;
						runner->next = NULL;
						strcpy(runner->filename, name);
					}
				}
			}
		}
		numFiles = numFiles + 3;	//account for first 3 argumetns (program name, flag, archive)
		//now populate args arguent to send to add files
		char *args[numFiles];
		//char myArray[numFiles][16];
		//args = myArray;
		for (int i = 0; i < numFiles; ++i){
			args[i] = malloc(sizeof(16));
			for (int j = 0; j < 16; ++j){
				args[i][j] = '\0';
			}
		}
		for (int i = 0; i < 3; ++i){
			strcpy(args[i], argv[i]);
		}
		int index = 3;
		runner = head;
		while(runner != NULL){
			strcpy(args[index++], runner->filename);
			runner = runner->next;
		}
		appendFiles(numFiles, args);
		closedir(directory);
	}
	return;

}
void printVerbose(int argc, char *argv[]){
	//open archive and set offset to first header
	char* archiveName = argv[2];
	struct stat archStats;
	int exists = stat(archiveName,&archStats);
	if (exists == -1){
		printf("Error, archive not found\n");
		return;
	}
	int archDesc = open(archiveName, O_RDONLY);
	if (lseek(archDesc, 8, SEEK_SET) < 0){
		printf("Error with archive file.\n");
		return;
	}

	char header[61];	//hold header
	int n = read(archDesc, header, 60);
	header[60] = '\0';
	if (n < 60 && n > 0) {
		printf("Error in archive formatting\n");
		return;	//archive does not contain any headers
	}
	while (n > 0){
		//first get and print permissions
		char mode[9];
		for (int i = 0; i < 8; ++i){
			mode[i] = header[i+40];
		} mode[8] = '\0';
		int octalPerm;
		sscanf(mode, "%o", &octalPerm);
		printPermTextFromOctal(octalPerm);
		printf("  ");

	    //now print owner ID and Group ID	    
	    char ownerID[7];
		for (int i = 0; i < 6; ++i){
			ownerID[i] = header[i+28];
		} ownerID[6] = '\0';
		uid_t ownerIDNum = atoi(ownerID);
		sprintf(ownerID, "%d", ownerIDNum);
		for (int i = 0; i < 6 - strlen(ownerID); ++i){
			printf(" ");
		}
		printf("%s/", ownerID);

		//trimWhitespace(ownerID);
		char groupID[7];
		for (int i = 0; i < 6; ++i){
			groupID[i] = header[i+34];
		} groupID[6] = '\0';
		gid_t groupIDNum = atoi(groupID);
		sprintf(groupID, "%d", groupIDNum);
		printf("%s", groupID);
		for (int i = 0; i < 6 - strlen(groupID); ++i){
			printf(" ");
		}

		trimWhitespace(groupID);
		//printf("%d/%d", ownerIDNum, groupIDNum);

		//print size
		char size[11];
		for (int i = 0; i < 10; ++i){
			size[i] = header[i+48];
		} size[10] = '\0';
		int contentSize = atoi(size);
		trimWhitespace(size);
		char leftAligned[11];
		for (int i = 0; i < 10; ++i){	//fill with spaces
			leftAligned[i] = ' ';
		}
		leftAligned[10] = '\0';
		for (int i = 0; i < strlen(size); --i){	//fill in numbers from left
			leftAligned[10 - i] = size[strlen(size) - 1 - i];
		}
		printf("%s", leftAligned);

		//print time
		char timestamp[13];
		for (int i = 0; i < 12; ++i){
			timestamp[i] = header[i+16];
		} timestamp[12] = '\0';
		trimWhitespace(timestamp);
		time_t tTime = atoi(timestamp);
		struct tm *timeBuff;
		time(&tTime);
		timeBuff = localtime(&tTime);
		char date[18];
		date[17] = '\0';
		char * format = "%b %d %H:%M %Y";
		strftime(date, 17, format, timeBuff);
		printf(" %s ", date);

		char fileName[17];
		fileName[16] = '\0';
		for (int i = 0; i < 16; ++i){
			fileName[i] = header[i];
		}
		trimWhitespace(fileName);
		printf("%s\n",fileName);
		//adjust for even text alignment
		if (contentSize % 2 == 1){
			contentSize = contentSize + 1;
		}
		if (lseek(archDesc, contentSize, SEEK_CUR) < 0){
			printf("Error with archive file.\n");
			return;
		}

		//read next header
		n = read(archDesc, header, 60);
		header[60] = '\0';
	}
}
void delete(int argc, char *artv[]){
	//not in function
}

void printPermTextFromOctal(mode_t mode){
	    printf( (mode & S_IRUSR) ? "r" : "-");
	    printf( (mode & S_IWUSR) ? "w" : "-");
	    printf( (mode & S_IXUSR) ? "x" : "-");
	    printf( (mode & S_IRGRP) ? "r" : "-");
	    printf( (mode & S_IWGRP) ? "w" : "-");
	    printf( (mode & S_IXGRP) ? "x" : "-");
	    printf( (mode & S_IROTH) ? "r" : "-");
	    printf( (mode & S_IWOTH) ? "w" : "-");
	    printf( (mode & S_IXOTH) ? "x" : "-");
}
