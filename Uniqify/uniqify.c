/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin

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

int fileToPipe(char * filename, int fd);

char *useage = "Usage: ./uniqify filename\nOnly one file can be passed, and it must be a text file,\n";


/*MARK: Begin Functions*/
int main(int argc, char *argv[]){

	if (argc != 2){
		printf("%s",useage);
		return 0;
	}

	//1) Create Pipe (parser to sorter and sorter to suppressor)
	int pfd1[2];
	int pfd2[2];
	pipe(pfd1);	//create pipe for process -> sort
	pipe(pfd2);	//create pipe for sort -> suppress

	//2) Create Child for Sorting
	int pid1, pid2;
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
		char str[37]; //max word size is 35 chars + null
		char last[37];
		last[0] = '\0';
		int count = 1;
		char num[6];
		FILE *stream = fdopen(pfd2[0], "r");
		while ((fgets(str, sizeof(str), stream))){	//words 35+ chars already pruned by parser
			if ((strcmp(str, last)) == 0){
				count++; continue;	//skip duplicates
			}
			//if not a duplicate, send previous word to stdout, if it is valid
			if ((strlen(last) >= 5)){	//only take words >= 5 chars
				snprintf(num, sizeof(num),"%5d", count); //turn count to string
				fputs(num, stdout);	//send count
				fputs(last, stdout); //send to standard out
				count = 1;		//reset count
			}
			strcpy(last, str); //copy unique strings into 'last'
		}

		//add final word
		if ((strlen(last) >= 5)){
			snprintf(num, sizeof(num),"%5d", count);
			fputs(num, stdout);	
			fputs(last, stdout); 
			count = 0;
		}

		fclose(stream);
		close(pfd2[0]);
		return 0;
	}

 	//4) Run the Parsing Parent
	close(pfd1[0]);	//close extra pipes
	close(pfd2[0]);
	close(pfd2[1]);
	char* filename = argv[1];
	char* extension = strchr(filename, '.');
	if (strcmp(extension, ".txt") != 0){
		printf("%s", useage);
		return 0;
	}
	fileToPipe(filename, pfd1[1]); //writes to the pipe until file end
	int *status1 = malloc(sizeof(int));
	int *status2 = malloc(sizeof(int));
	waitpid(pid1, status1, 0);
	waitpid(pid2, status2, 0);
	return 0;
}
int fileToPipe(char * filename, int fd){
	//Given a filename and a file descriptor to write to, this function
	//writes all of the words less than 36 characters in the file
	//to the file descriptor in lowercase letters followed by new line characters
	//
	
	//Open file for reading
	FILE *readstream;
	FILE *writestream;
	readstream = fopen(filename, "r");
	if (readstream == NULL){
		printf("Error opening file\n");
		return -1;
	}
	writestream = fdopen(fd, "w");

	char c;
	int letterCount = 0;	//denotes length of current word being read
	do{	//read file until empty
		c = fgetc(readstream);
		if (feof(readstream)){	//end of file
			break;
		}else if (isalpha((int)c)){	//write to sorting process
			letterCount++;
			if (letterCount <= 35){	//skip alpha letters after 35 writes
				fputc(tolower(c), writestream);
			}
		}else if (letterCount > 0){	//time to end word
			letterCount = 0;
			fputc('\n', writestream);	//terminate word
		}else{	//not a letter and a word has not started
			continue;
		}
	}while(1);
	fclose(readstream);
	fclose(writestream);
	close(fd);
	return 1;
}



