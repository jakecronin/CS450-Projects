#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

void hi(int signum);
int main(int argc, char const *argv[]){

	signal(SIGQUIT, hi);
	signal(SIGHUP, hi);
	signal(SIGPIPE, hi);

	printf("beginning writer\n");
	if (mkfifo("/tmp/jakefifo", 0666) == -1 && errno != EEXIST){
		perror("making fifo"); exit(1);}

	int fifofd;
	printf("going to open writer\n");
	while((fifofd = open("/tmp/jakefifo", O_WRONLY)) == -1){
		if (errno == ENXIO){
			printf("writer can't open\n"); sleep(5);}
		else{perror("opening fifo"); exit(2);}
	}
	printf("writer open\n");

	while(1){
		char c;
		c = getc(stdin);
		if (c == 'q'){break;}
		while(write(fifofd, &c, 1) == -1){
			printf("waiting to write\n");
		}
	}
	close(fifofd);
	return 0;
}

void hi(int signum){
	printf("caught %d\n", signum);
	double num;
	for (int i = 0; i < 1000000000; ++i){
		num = i * 18; 
	}
	printf("%d handler ended\n", signum);
}