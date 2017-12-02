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
//#include <sys/msg.h>; 
#include <mqueue.h>	//use posix message queue

int testPerfect(int n);	//returns number if perfect, -1 if not perfect
char *useage = "Usage: ./compute integer\n";

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

int pid;

int main(int argc, char *argv[]){
	if (argc != 2){
		printf("%s",useage);
		return 0;
	}
	int key = 60302;
	pid = getpid();
	int processIndex = 0;	//index of this process in shared memory process array
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

	//Tell Manager to make entry into process seciton of shared memory
	//open/create message queue
	char * mqname = "jakequeue";
	mqd_t mqd;
	if ((mqd = mqopen(mqname, O_WRONLY)) == -1){
		printf("Error opening message queue in Compute\n");
		exit(1);
	}

	//send message to manager
	char * message;
	int size = 1;
	size = snprintf(message, size, "begin %d", pid);	//query necessary size
	snprintf(message, size, "begin %d", pid);
	mq_send(mqd, message, size, 1);

	//Wait until message is processed and we know our index in process table to begin
	while((processIndex = getProcessIndex(shmaddr)) == -1){
		wait(1);
		printf("process %d waiting to enter process table\n", pid);
	}

	//Process numbers beginning with starting point
	int startNumber = atoi(argv[1]);
	int curr = startNumber;
	do{
		if (!isTested(shmaddr, curr)){ //check if number is in bitmap
			setTested(shmaddr, curr);
			incrementTested(shmaddr, curr);
			int isPerfect = testPerfect(curr); 	//process number
			if (isPerfect){
				size = snprintf(message, 1, "perf %d", curr);
				snprintf(message, size, "perf %d", curr);
				mq_send(mqd, message, size, 1);
				incrementFound(shmaddr);
			}
		}else{
			incrementSkipped(shmaddr);
		}
		curr = startNumber;
	}while(curr != startNumber);

	//detach shared memory
	shmdt(shmaddr);

	//close  message queue
	mq_close(mqd);
}

int getProcessIndex(shm_buf * address){
	for (int i = 0; i < 20; ++i){
		process * proc = address->processes[i];
		if (proc){
			if (proc->pid == pid){
				return i;
			}
		}
	}
	return -1;
}
int incrementTested(shm_buf * address){
	return 0;
}
int incrementSkipped(shm_buf * address){
	return 0;
}
int incrementFound(shm_buf * address){
	return 0;
}
int setTested(shm_buf * address, int val){
	return 0;
}
int isTested(shm_buf * address, int test){
	return 0;
}

int testPerfect(int n){
	int sum = 0;
	for (int i = 1; i < n; ++i){	//test all numbers 1 to n-1
		if (((n%i)==0)){	//this is a divisor
			sum += i;
		}
	}
	if (sum == n){
		return n;
	}else{
		return -1;
	}
}