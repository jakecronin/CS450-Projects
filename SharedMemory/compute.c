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

int testPerfect(int n);	//returns number if perfect, -1 if not perfect
char *useage = "Usage: ./compute integer\n";

typedef struct _msg{
	long mtype;
	int mdata;
}msgbuf;

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
shm_buf * shmaddr;	//address of shared memory
int processIndex;	//index of this process in shared memory process array
int msqid;
int pid;

int incrementTested(shm_buf * address);
int incrementSkipped(shm_buf * address);
int incrementFound(shm_buf * address);
int setTested(shm_buf * address, int val);
int isTested(shm_buf * address, int test);
int testPerfect(int n);
int getProcessIndex(shm_buf * address);
void die(int signum);
int main(int argc, char *argv[]){
	if (argc != 2){
		printf("%s",useage);
		return 0;
	}

	//Handle signals -> delete entry in process table, detach shared mem, die
	signal(SIGINT, die);
	signal(SIGHUP, die);
	signal(SIGQUIT, die);

	/* GET AND ATTACH SHARED MEMORY*/
	//---------------------------------------------------------------
	int key = 60302;
	pid = getpid();
	//create or find + link shared 
	long shmsize = sizeof(shm_buf);	//size of shared memory
	if ((shmid = shmget(key, shmsize, IPC_CREAT | 0666)) < 0){	//find or create shared memory
		printf("Error getting shared memory, errno %d\n", errno);
		exit(1);
	}

	if ((shmaddr = shmat(shmid, NULL, 0)) == (int *) -1){	//attach shared memory at next open address for reading and writing
		exit(1);
	}

	
	/* GET MESSAGE QUEUE AND CHECK IN WITH MANAGER */
	//------------------------------------------------------------------------
	//Tell Manager to make entry into process seciton of shared memory
	//open/create message queue
	key_t msgkey;
	int flags = IPC_CREAT;
	msgkey = ftok(".", 'j');

	if ((msqid = msgget(key, flags | 0666)) == -1){
		printf("Error opening message queue in Compute\n");
		exit(1);
	}

	//send pid to manager
	msgbuf message;
	message.mdata = pid;
	message.mtype = 1;	//type 1 means its a pid
	printf("prepping to send message: %d\n", message.mdata);
	if ((msgsnd(msqid, &message, sizeof(message.mdata), 0)) < 0){
		printf("Error sending message. Errno: %d\n", errno);
	}else{
		printf("finished sending message: %d of type %lu\n", message.mdata, message.mtype);
	}
		
	//Get Process index from manager
	if ((msgrcv(msqid, &message, sizeof(message.mdata), pid, 0)) == -1){
		printf("Error getting process index from manager. Errno: %d\n", errno);
		exit(1);
	}
	processIndex = message.mdata;
	printf("got process index: %d\n", processIndex);

	//Process numbers beginning with starting point
	int startNumber = atoi(argv[1]);
	int curr = startNumber;
	do{
		if (!isTested(shmaddr, curr)){ //check if number is in bitmap
			setTested(shmaddr, curr);
			incrementTested(shmaddr);
			int isPerfect = testPerfect(curr); 	//process number
			if (isPerfect){
				incrementFound(shmaddr);
			}
		}else{
			incrementSkipped(shmaddr);
		}
		curr = startNumber;
	}while(curr != startNumber);

}

int getProcessIndex(shm_buf * address){
	for (int i = 0; i < 20; ++i){
		process * proc = &(address->processes[i]);
		if (proc){
			if (proc->pid == pid){
				return i;
			}
		}
	}
	return -1;
}
int incrementTested(shm_buf * address){
	address->process[processIndex].tested++;
}
int incrementSkipped(shm_buf * address){
	address->process[processIndex].skipped++;
}
int incrementFound(shm_buf * address){
	address->process[processIndex].found++;
}
int setTested(shm_buf * address, int val){
	int index = val / (sizeof(int)*8);
	int offset = val % (sizeof(int)*8);
	address->bitmap[index] = address->bitmap[index] | (1 << offset);
	return 1;
}
int isTested(shm_buf * address, int test){
	int index = val / (sizeof(int)*8);
	int offset = val % (sizeof(int)*8);
	return (address->bitmap[index] | (1 << offset));
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
void die(int signum){
	printf("compute received signal %d\n", signum);
	sleep(3);
	//delete process table entry
	shmaddr->processes[processIndex].pid = 0;
	shmaddr->processes[processIndex].tested = 0;
	shmaddr->processes[processIndex].skipped = 0;
	shmaddr->processes[processIndex].found = 0;

	//detach shared memory
	shmdt(shmaddr);

	exit(0);
}


