/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <limits.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <string.h> 



char *useage = "Usage: ./report [-k]\n -k flag is optional. Signals to shut down computation\n";

typedef struct _process{
	int pid;
	int tested;
	int skipped;
	int found;
}process;


typedef struct _shm_buf{
	int bitmap[1048576];	//holds 2^25 bits (2^25 / CHAR_BIT / sizeof(int)
	int perfs[20];
	process processes[20];
	int managepid;
}shm_buf;

int shmid;		//shared memory id
shm_buf * shmaddr;		//shared memory address

int main(int argc, char *argv[]){

	int k = 0;
	if (argc == 2){
		char* flag = argv[1];
		if (strlen(flag) != 2){
			printf("Invalid flag\n%s",useage);
			exit(1);
		}
		if (flag[0] == '-' && flag[1] == 'k'){
			k = 1;
		}else{
			printf("%s\n", useage);
			exit(1);
		}
	}else if (argc != 1){
		printf("%s",useage);
		return 0;
	}


	/* GET AND ATTACH SHARED MEMORY */
	//------------------------------------------------------------------------
	int key = 60302;
	int shmsize = sizeof(shm_buf);	//size of shared memory
	if ((shmid = shmget(key, shmsize, 0666)) < 0){	//find or create shared memory
		printf("Shared memory doesn't exist. Nothing to report\n");
		exit(1);
	}
	if ((shmaddr = shmat(shmid, NULL, 0)) == (int *) -1){	//attach shared memory at next open address for reading and writing
		exit(1);
	}

	//* READ SHARED MEMORY *//
	//------------------------------------------------------------
	//first read number of perfects found
	//second report each process currently computing tested, skipped, found
	//third report sum of tested, skipped, found

	//1) Report Perfects Found
	int numPerfs = 0;
	for (int i = 0; i < 20; ++i){
		if(shmaddr->perfs[i] != 0){
			numPerfs++;
		}
	}
	printf("Found %d Perfects\n\n", numPerfs);

	//2) Report on each process
	process proc;
	int pid=0, tested=0, skipped=0, found=0;
	for (int i = 0; i < 20; ++i){
		proc = shmaddr->processes[i];
		if(proc.pid != 0){
			pid++;
			tested += proc.tested;
			skipped += proc.skipped;
			found += proc.found;
			printf("Process PID=%d Tested=%d Skipped=%d Found=%d\n", proc.pid, proc.tested, proc.skipped, proc.found);
		}
	}
	
	//3) Report total values
	printf("\n Total Processing: \n\tRunning Compute Processes: %d\n\ttested=%d\n\tskipped=%d\n\tfound=%d\n", pid, tested, skipped, found);

	
	if (k){
		if ((shmaddr->managepid) == 0){
			printf("-k flag invoked, but no manage process found\n");
		}else{
			kill(shmaddr->managepid, SIGINT);
		}
	}
	//detach shared memory
	shmdt(shmaddr);

	return 0;
}












