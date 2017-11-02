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

int parse(char *filename);

char *useage = "Usage: ./uniqify filename\nOnly one file can be passed, and it must be a text file,";


/*MARK: Begin Functions*/
int main(int argc, char *argv[]){

	if (argc != 2){
		printf("%s",useage);
		return 0;
	}

	char* filename = argv[1];
	char* extension = strchr(filename, '.');
	if (strcmp(extension, ".txt") != 0){
		printf("%s", useage);
		return 0;
	}

	parse(filename);

}

int parse(char * filename){
	int charCount = 0;
	FILE *stream;
	char c;

	stream = fopen(filename, "r");
	if (stream == NULL){
		printf("Error opening file\n");
		return -1;
	}

	do{
		c = fgetc(stream);
		if (feof(stream)){
			break;
		}
		printf("read %c\n", c);
	}while(1);
	return 1;
}




