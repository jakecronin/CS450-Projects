#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void appendFiles(int argc, char *argv[]);
void addWhitespace(char *str, int size);
char *useage = "Usage: ./myar key afile filename ...\nq\tquickly append named files to archive\nx\textract named files\nt\tprint a concise table of contents of the archive\nv\tprint a verbose table of contents of the archive\nd\tdelete named files from archive\nA\tquickly append \"ordinary\" files in the current directory that have been modified within the last two hours\n";


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
		printf("flag is a q\n");
		appendFiles(argc, argv);
		break;
		case 'x':
		printf("flag is an x\n");
		break;
		case 't':
		printf("flag is a t\n");
		break;
		case 'v':
		printf("flag is a v\n");
		break;
		case 'd':
		printf("flag is a d\n");
		break;
		case 'A':
		printf("flag is an A\n");
		break;
		default:
		printf("Invalid flag\n%s", useage);
	}
}

void appendFiles(int argc, char *argv[]){
	printf("in append files\n");
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
			printf("Invalid not found: %s\n", fileName);
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
		printf("need to create archive file\n");
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
		n = sprintf(fileSize, "%lld", fileStats.st_size);
		addWhitespace(fileSize, 11);
		strcat(header, fileSize);

		header[58] = '\x60';
		header[59] = '\x0A';
		header[60] = '\0';

		//Add header to write contents
		strcat(toWrite, header);
		printf("toWrite after adding file: %s\n", toWrite);

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

	printf("about to write: %s\n", toWrite);
	int written = write(archDesc, toWrite, bytes);
	printf("successfully wrote %d bytes\n",written);
	close(archDesc);
}
void addWhitespace(char* str, int size){
	int len = strlen(str);
	for (int i = len; i < size; ++i){
		str[i] = ' ';
	}
	str[size - 1] = '\0';
}
void extract(int argc, char *argv[]){
	//extracts all individual fiels if no arguments given
	//extracts only named files if there are given names

}




