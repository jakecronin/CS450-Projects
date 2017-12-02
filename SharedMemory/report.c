/*
THIS CODE IS MY OWN WORK AND I WROTE IT WITHOUT CONSULTING A TUTOR
-Jake Cronin

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <limits.h>
#include <math.h>
#include <sys/ipc.h>; 
#include <sys/msg.h>; 



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
}shm_buf;


int main(int argc, char *argv[]){
	if (argc == 2){
		if (strcmp(argv[1], "-k")){
			//do kill stuff
			printf("filler\n");
		}else{
			printf("%s\n", useage);
		}
	}else if (argc != 1){
		printf("%s",useage);
		return 0;
	}


	//* GET SHARED MEMORY *//
	//------------------------------------------------------------
	int key = 60302;

	//create or find + link shared 
	shm_buf * shmaddr;	//address of shared memory
	int shmid;		//shared memory id
	long shmsize = sizeof(shm_buf);	//size of shared memory
	if ((shmid = shmget(key, shmsize, IPC_CREAT | 0666)) < 0){	//find or create shared memory
		printf("Error getting shared memory\n");
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
		if(shmaddr->perfs[i]){
			numPerfs++;
		}
	}
	printf("Found %d Perfects\n", numPerfs);

	//2) Report on each process
	process * proc;
	int pid, tested, skipped, found;
	for (int i = 0; i < 20; ++i){
		proc = shaddr->process[i];
		if(proc){
			pid += proc->pid;
			tested += proc->tested;
			skipped += proc->skipped;
			found += proc->found;
			printf("Process PID=%d tested=%d skipped=%d found=%d\n", proc->pid, proc->tested, proc->skipped, proc->found);
		}
	}
	
	//3) Report total values
	printf("\n Total Processing: tested=%d skipped=%d found=%d\n", proc->pid, proc->tested, proc->skipped, proc->found);

	
	//detach shared memory
	shmdt(shmaddr);

	return 0;
}












