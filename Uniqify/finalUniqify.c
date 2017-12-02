/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin

EXTRA CREDIT:
- sorting is distributed among 2 processes

Uniqify
Read a text file and output the unique words in a file, sorted in alphabetic order
- parse words greater than or equal to 5 characters
- truncate words over 35 characters (38 character word gets last 3 chars cut off)
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
#include <ctype.h>

int fileToPipes(int fdin, int fd, int fd2);

char *useage = "Usage: ./uniqify <file.txt\nOnly one file can be passed, and it must be a text file,\n";

typedef struct _node{
	char filename[17];
	struct _node *next;
	struct _node *prev;
}node;

/*MARK: Begin Functions*/
int main(int argc, char *argv[]){

	if (argc != 1){
		printf("%s",useage);
		return 0;
	}

	//1) Create Pipe (parser to sorter and sorter to suppressor)
	int pfd1[2];
	int pfd2[2];
	int altpfd1[2];
	int altpfd2[2];
	pipe(pfd1);	//create pipe for process -> sort
	pipe(pfd2);	//create pipe for sort -> suppress
	pipe(altpfd1); //create second pipe for process -> sort2
	pipe(altpfd2); //second pipe for sort2 -> suppress

	//2) Create Child for Sorting
	int pid1, pid2, pid3;
	pid1 = fork();
	switch(pid1){
	case -1:
		printf("Error running process. Terminating.\n");
		return 0;
	case 0: //this is the sorting child		
		dup2(pfd1[0], 0);	//redirect standard in to pfd1 read
		dup2(pfd2[1], 1);	//redirect standard out to pfd2 write
		close(pfd1[0]);	//close extra file descriptors
		close(pfd1[1]);
		close(pfd2[0]);
		close(pfd2[1]);
		close(altpfd1[0]);
		close(altpfd1[1]);
		close(altpfd2[0]);
		close(altpfd2[1]);
		execlp("/usr/bin/sort", "sort", (char *)NULL);	//run /bin/sort
		return 0;
	}

	//2.5) Create second Child for Sorting
	pid3 = fork();
	switch(pid3){
	case -1:
		printf("Error running process. Terminating.\n");
		return 0;
	case 0: //this is the sorting child		
		dup2(altpfd1[0], 0);	//redirect standard in to pfd1 read
		dup2(altpfd2[1], 1);	//redirect standard out to pfd2 write
		close(altpfd1[0]);	//close extra file descriptors
		close(altpfd1[1]);
		close(altpfd2[0]);
		close(altpfd2[1]);
		close(pfd1[0]);
		close(pfd1[1]);
		close(pfd2[0]);
		close(pfd2[1]);
		execlp("/usr/bin/sort", "sort", (char *)NULL);	//run /bin/sort
		return 0;
	}
	//3) Create Suppressing Child
	pid2 = fork();
	switch(pid2){
	case -1:
		return 0;
	case 0:	//suppressing child
		close(pfd1[0]); //close extra file descriptors
		close(pfd1[1]);
		close(pfd2[1]);
		close(altpfd1[0]);
		close(altpfd1[1]);
		close(altpfd2[1]);
		char str[37]; //max word size is 35 chars + null
		char str2[37]; //max word size is 35 chars + null
		str[0] = '\0';
		str2[0] = '\0';
		char last[37];
		last[0] = '\0';
		int count = 1;
		char num[6];
		FILE *stream = fdopen(pfd2[0], "r");
		FILE *stream2 = fdopen(altpfd2[0], "r");

		int first = 1;
		int second = 1;
		fgets(str, sizeof(str), stream);	//get first words from both streams
		fgets(str2, sizeof(str2), stream2);
		while(1){
			while(str[0] != '~' && strlen(str) < 5){
				if ((fgets(str, sizeof(str), stream) == NULL)){
					str[0] = '~';
				}
			}
			while(str2[0] != '~' && strlen(str2) < 5){
				if((fgets(str2, sizeof(str2), stream2) == NULL)){
					str2[0] = '~';
				}
			}
			if (((str[0] == '~') && (str2[0] == '~'))){
				break;
			}
			int comp = strcmp(str, str2);
			if ((comp < 0)){
				if ((strcmp(str, last)) == 0){
					count++; str[0] = '\0';
					continue;	//skip duplicates
				}
				//otherwise, string is unique, send to stdout
				if ((strlen(last) >= 5)){
					snprintf(num, sizeof(num),"%5d", count); //turn count to string
					fputs(num, stdout);	//send count
					fputs(last, stdout); //send to standard out
					count = 1;		//reset count
				}
				strcpy(last, str); //copy unique strings into 'last'
				str[0] = '\0';
				continue;
			}else{	//str2 comes first
				if ((strcmp(str2, last)) == 0){
					count++; 
					str2[0] = '\0';
					continue;	//skip duplicates
				}
				//otherwise, string is unique, send to stdout
				if ((strlen(last) >= 5)){
					snprintf(num, sizeof(num),"%5d", count); //turn count to string
					fputs(num, stdout);	//send count
					fputs(last, stdout); //send to standard out
					count = 1;		//reset count
				}
				strcpy(last, str2); //copy unique strings into 'last'
				str2[0] = '\0';
				continue;
			}
		}

		//add final word
		if ((strlen(last) >= 5)){
			snprintf(num, sizeof(num),"%5d", count);
			fputs(num, stdout);	
			fputs(last, stdout); 
			count = 0;
		}
		fclose(stream);
		fclose(stream2);
		close(pfd2[0]);
		close(altpfd2[0]);
		return 0;
	}

 	//4) Run the Parsing Parent
	close(pfd1[0]);	//close extra pipes
	close(pfd2[0]);
	close(pfd2[1]);
	close(altpfd1[0]);
	close(altpfd2[0]);
	close(altpfd2[1]);

	//char* filename = argv[1];
	//char* extension = strchr(filename, '.');
	//if (strcmp(extension, ".txt") != 0){
	//	printf("%s", useage);
	//	return 0;
	//}
	fileToPipes(0, pfd1[1], altpfd1[1]); //writes to the pipe until file end
	int *status1 = malloc(sizeof(int));
	int *status2 = malloc(sizeof(int));
	int *status3 = malloc(sizeof(int));
	waitpid(pid1, status1, 0);
	waitpid(pid2, status2, 0);
	waitpid(pid3, status3, 0);
	return 0;
}
int fileToPipes(int fdin, int fd, int fd2){
	//Given a filename and a file descriptor to write to, this function
	//writes all of the words less than 36 characters in the file
	//to the file descriptor in lowercase letters followed by new line characters
	//
	
	//Open file for reading
	FILE *readstream;
	FILE *writestream;
	FILE *writestream2;
	//readstream = fopen(filename, "r");
	readstream = fdopen(fdin, "r");
	if (readstream == NULL){
		printf("Error accessing standard in\n");
		return -1;
	}
	writestream = fdopen(fd, "w");
	writestream2 = fdopen(fd2, "w");

	char c;
	int letterCount = 0;	//denotes length of current word being read
	int n = 1;	//which sorter to write to, 1 or -1
	do{	//read file until empty
		c = fgetc(readstream);
		if (feof(readstream)){	//end of file
			break;
		}else if (isalpha((int)c)){	//write to sorting process
			letterCount++;
			if (letterCount <= 35){	//skip alpha letters after 35 writes
				if ((n > 0))
					fputc(tolower(c), writestream);
				else
					fputc(tolower(c), writestream2);
			}
		}else if (letterCount > 0){	//time to end word
			letterCount = 0;
			if ((n>0))
				fputc('\n', writestream);	//terminate word
			else
				fputc('\n', writestream2);
			n = n * -1;	//alternate sorting processes for each word
		}else{	//not a letter and a word has not started
			continue;
		}
	}while(1);
	fclose(readstream);
	fclose(writestream);
	fclose(writestream2);
	close(fd2);
	close(fd);
	return 1;
}



