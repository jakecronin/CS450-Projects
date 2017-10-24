/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin

Extra Credit Attempts:
	1) -q supports multiple files on command line
	2) -x suppots multiple files on command line
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

void appendFiles(int argc, char *argv[]);
void addWhitespace(char *str, int size);
void extract(int argc, char *argv[]);
void trimWhitespace(char* str);
void printTable(int argc, char *argv[]);
void addAllRecent(int argc, char *argv[]);
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
		printf("v flag not in use\n");
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

	//Calculate Write Size
	int bytes = 0; //aggregate number of bytes to write to file
	//for (int i = argc - 1; i >= 3; i--){	//for each file, got contents size
	for (int i = 3; i < argc; i++){
		char* fileName = argv[i];
		struct stat fileStats;
		int exists = stat(fileName, &fileStats);
		if (exists == -1){
			//printf("Invalid not found: %s\n", fileName);
			continue;
		}
		bytes = bytes + 60;	//add 60 bytes for file header
		bytes = bytes + fileStats.st_size;	//add file size
		if (fileStats.st_size % 2){
			bytes = bytes + 1; //add buffer \n character to keep data 2 byte aligned
		}
	}
	char* archiveName = argv[2];
	struct stat archStats;
	int exists = stat(archiveName,&archStats);
	if (exists == -1){
		bytes = bytes + 8;	//add bytes for archive header
	}

	//BUILD STRING TO WRITE TO FILE
	char toWrite[bytes + 1];	//aggregate string of content to be written
	for (int i = 0; i < bytes + 1; ++i){
			toWrite[i] = '\0';
	}
	int archDesc;
	if (exists == -1){	//add archive header if necessary
		archDesc = open(archiveName, O_CREAT | O_RDWR | O_TRUNC, 0666);
		//printf("need to create archive file\n");
		char* archHeader = "!<arch>\n";
		for (int i = 0; i < 8; ++i){
			toWrite[i] = archHeader[i];
		}
		toWrite[8] = '\0';
	}else{
		archDesc = open(archiveName, O_APPEND | O_WRONLY, 0666);
	}
	//Add header + body for each file
	//for (int i = argc - 1; i >= 3; i--){
	for (int i = 3; i < argc; i++){
		char* fileName = argv[i];
		struct stat fileStats;
		int exists = stat(fileName, &fileStats);
		if (exists == -1){
			//printf("Invalid not found: %s\n", fileName);
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
		n = sprintf(fileSize, "%lld", fileStats.st_size);
		addWhitespace(fileSize, 11);
		strcat(header, fileSize);

		header[58] = '\x60';
		header[59] = '\x0A';
		header[60] = '\0';

		//Add header to write contents
		strcat(toWrite, header);
		//Second get file contents
		char fileContents[fileStats.st_size];
		int fileDesc = open(fileName, O_RDONLY);
		n = read(fileDesc, fileContents, fileStats.st_size);

		int len = strlen(toWrite);
		for (int i = 0; i < fileStats.st_size; ++i){
		 	toWrite[len++] = fileContents[i];
		}
		if (fileStats.st_size % 2){
			toWrite[len++] = '\n';	//buffer to keep data 2 byte aligned
		}
		toWrite[len++] = '\0';
		//fileContents[fileStats.st_size] = '\0';
		// strcat(toWrite, fileContents);	//add file contents onto header
		// if (fileStats.st_size % 2){
		// 	strcat(toWrite, "\n");	//buffer to keep data 2 byte aligned
		// }
		close(fileDesc);
	}
	toWrite[bytes] = '\0';	//make sure it is null terminated
	int written = write(archDesc, toWrite, bytes);
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
	//build linkedlist of filenames
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
		while(runner != NULL){
			fileName = runner->filename;
			int match = 1;
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
		
		//get parts of header and file contents
		trimWhitespace(fileName);

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
		//printf("first four chars are %s\n", perms);
		//long int strtol() octalPerms;
		//sscanf(perms, "%o", octalPerms);
		//printf("octal perms: %d\n", octalPerms);
		int permVal = atoi(perms);
		int modeVal = atoi(mode);
		mode_t modeT = modeVal;
		char size[11];
		for (int i = 0; i < 10; ++i){
			size[i] = header[i+48];
		} size[10] = '\0';
		int contentSize = atoi(size);
		if (contentSize % 2 == 1){
			contentSize = contentSize + 1;
		}
		char content[contentSize + 1];
		n = read(archDesc, content, contentSize);
		if (n < contentSize){
			printf("could not load full content. Archive file not formatted correctly\n");
			return;
		}
		if (contentSize % 2 == 1){
			content[contentSize - 1] = '\0';
		}else{
			content[contentSize] = '\0';
		}

		if (runner != NULL){	//if found match, remove link and create file
			if (runner == head){
				head = runner->next;
			}
			if (runner->prev != NULL){
				runner->prev->next = runner->next;
			}
			if (runner->next != NULL){
				runner->next->prev = runner->prev;
			}
			free(runner);
			int fileDesc = open(fileName, O_TRUNC | O_CREAT | O_RDWR, modeT);
			n = write(fileDesc, content, contentSize);
			chmod(fileName, modeT);
			chown(fileName, ownerIDNum, groupIDNum);
			//write(fileDesc, content, contentSize);
			utime(fileName, timeBuff);
			close(fileDesc);
			//printf("mode string is %s\n", mode);
			//printf("perm val is %d\n", permVal);
			//printf("mode val is %d\n", modeVal);
			//printf("perm string is %s\n", perms);
			//printf("modeT is %d\n", modeT);
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


	//build linkedlist that populates as we run through archive
	node * head = NULL;
	head = malloc(sizeof(node));
	head->next = NULL;
	head->prev = NULL;
	node * runner = head;
	char header[61];	//get first header
	int n = read(archDesc, header, 60);
	if (n < 60) {
		printf("Error in archive formatting\n");
		return;	//archive does not contain any headers
	}
	header[60] = '\0';
	for (int i = 0; i < 16; ++i){
		head->filename[i] = header[i];
	} head->filename[16] = '\0';
	trimWhitespace(head->filename);
	char size[11];
	for (int i = 0; i < 10; ++i){
		size[i] = header[i+48];
	} size[10] = '\0';
	int contentSize = atoi(size);
	if (lseek(archDesc, contentSize, SEEK_CUR) < 0){
		printf("Error with archive file.\n");
		return;
	}
	while(1){	//continue until run through whole archive
		//get next header
		int n = read(archDesc, header, 60);
		if (n < 60) {
			while(runner != NULL){
				printf("%s\n", runner->filename);
				runner = runner->prev;
			}
			return;	//natural ending when end of archive is reached
		}
		//add new link
		runner->next = malloc(sizeof(node));
		runner->next->prev = runner;
		runner = runner->next;

		header[60] = '\0';
		for (int i = 0; i < 16; ++i){
			runner->filename[i] = header[i];
		} runner->filename[16] = '\0';
		trimWhitespace(runner->filename);
		char size[11];
		for (int i = 0; i < 10; ++i){
			size[i] = header[i+48];
		} size[10] = '\0';
		int contentSize = atoi(size);
		if (lseek(archDesc, contentSize, SEEK_CUR) < 0){
			printf("Error with archive file.\n");
			return;
		}
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


