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

int incrementTested();
int incrementSkipped();
int incrementFound();
int setTested(int val);
int isTested(int test);
int testPerfect(int n);
int getProcessIndex();
void die(int signum);
int sendPerfect(int val);
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
		die(SIGINT);
	}

	//send pid to manager
	msgbuf message;
	message.mdata = pid;
	message.mtype = 1;	//type 1 means its a pid
	if ((msgsnd(msqid, &message, sizeof(message.mdata), 0)) < 0){
		printf("Error sending message. Errno: %d\n", errno);
	}

	//Get Process index from manager
	processIndex = -1;
	if ((msgrcv(msqid, &message, sizeof(message.mdata), pid, 0)) == -1){
		printf("Error getting process index from manager. Errno: %d\n", errno);
		die(SIGINT);
	}
	processIndex = message.mdata;


	/* DO COMPUTING */
	//------------------------------------------------------------------------
	printf("Compute registered. Beginning computing\n");
	int startNumber = atoi(argv[1]);
	if (startNumber < 0){
		startNumber = 0;
	}
	int curr = startNumber;
	do{
		if (!isTested(curr)){ //check if number is in bitmap
			setTested(curr);
			incrementTested();
			int isPerfect = testPerfect(curr); 	//process number
			if (isPerfect){
				printf("got perfect %d\n", curr);
				incrementFound();
				sendPerfect(curr);
			}
		}else{
			incrementSkipped();
		}
		curr++;
		if (curr > pow(2,25)){
			curr = 0;
		}

	}while(curr != startNumber);

}

int getProcessIndex(){
	for (int i = 0; i < 20; ++i){
		process * proc = &(shmaddr->processes[i]);
		if (proc){
			if (proc->pid == pid){
				return i;
			}
		}
	}
	return -1;
}
int incrementTested(){
	return shmaddr->processes[processIndex].tested++;
}
int incrementSkipped(){
	return shmaddr->processes[processIndex].skipped++;
}
int incrementFound(){
	return shmaddr->processes[processIndex].found++;
}
int setTested( int val){
	int index = val / (sizeof(int)*8);
	int offset = val % (sizeof(int)*8);
	shmaddr->bitmap[index] = shmaddr->bitmap[index] | (1 << offset);
	return 1;
}
int isTested(int val){
	int index = val / (sizeof(int)*8);
	int offset = val % (sizeof(int)*8);
	int ans = (shmaddr->bitmap[index] & (1 << offset));
	return (shmaddr->bitmap[index] & (1 << offset));
}
int testPerfect(int n){
	if (n<6) return 0;
	int sum = 0;
	for (int i = 1; i < n; ++i){	//test all numbers 1 to n-1
		if (((n%i)==0)){	//this is a divisor
			sum += i;
		}
	}
	if (sum == n){
		return n;
	}else{
		return 0;
	}
}
int sendPerfect(int val){
	//send message to manager that a new perfect has been found
	msgbuf message;
	message.mdata = val;
	message.mtype = 2;	//type 2 means its a perfect
	if ((msgsnd(msqid, &message, sizeof(message.mdata), 0)) < 0){
		printf("Error sending message. Errno: %d\n", errno);
		return 0;
	}
	return 1;
}
void die(int signum){
	printf("compute received signal %d. Going to Die.\n", signum);
	//delete process table entry
	if (shmaddr){
		shmaddr->processes[processIndex].pid = 0;
		shmaddr->processes[processIndex].tested = 0;
		shmaddr->processes[processIndex].skipped = 0;
		shmaddr->processes[processIndex].found = 0;
		//detach shared memory
		shmdt(shmaddr);
	}

	if (processIndex == -1){	//manager not around, destroy manage queue and shared memory
		msgctl(msqid, IPC_RMID, NULL); 	//Close message queue
		shmctl(shmid, IPC_RMID, NULL);	//unlink shared memory so it is destroyed
	}

	exit(0);
}


