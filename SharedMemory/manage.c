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



char *useage = "Usage: ./manage\n";

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

shm_buf * shmaddr;		//shared memory address
int msqid;				//message queue id
int shmid;		//shared memory id

int addPidToTable(int pid);
int addPerfToTable(int val);
void die(int signum);

int main(int argc, char *argv[]){
	if (argc != 1){
		printf("%s",useage);
		return 0;
	}
	int key = 60302;

	//Handle signals -> tell computes to kill, sleep(5), then cleanup and die
	signal(SIGINT, die);
	signal(SIGHUP, die);
	signal(SIGQUIT, die);


	/* GET AND ATTACH SHARED MEMORY */
	//------------------------------------------------------------------------
	//create or find + link shared 
	//shm_buf * shmaddr;	//address of shared memory
	int shmsize = sizeof(shm_buf);	//size of shared memory
	if ((shmid = shmget(key, shmsize, IPC_CREAT | 0666)) < 0){	//find or create shared memory
		printf("Error getting shared memory errno: %d\n", errno);
		exit(1);
	}

	if ((shmaddr = shmat(shmid, NULL, 0)) == (int *) -1){	//attach shared memory at next open address for reading and writing
		exit(1);
	}

	if (shmaddr->managepid != 0){		//exit if another manage already exists
		//another manage process exists, end
		printf("Another manage process already exists. Exiting\n");
		exit(0);
	}
	//memset(shmaddr, 0, sizeof(shm_buf));
	shmaddr->managepid = getpid();	//set manage pid


	/* GET MESSAGE QUEUE AND BEGIN LISTENING */
	//------------------------------------------------------------------------
	key_t msgkey;
	int flags = IPC_CREAT;

	msgkey = ftok(".", 'j');

	if ((msqid = msgget(key, flags | 0666)) == -1){
		printf("Error opening message queue in manage, errno %d\n", errno);
		exit(1);
	}

	//Recieve messages
	msgbuf message;
	int isNew; //flag for check to ensure this perfect is unique. Used only for signal 2
	while(1){
		if ((msgrcv(msqid, &message, sizeof(message.mdata), -2, 0)) < 0){	//receive only type 1 and type 2 messages (pids and perfs)
			printf("Error recieving message. Errno: %d\n", errno);
			break;
		}
		switch(message.mtype){
			case 1:
				message.mtype = message.mdata;	//remember compute pid in mtype
				if ((message.mdata = addPidToTable(message.mdata)) == -1){
					printf("compute process %d exceeds process table. Killed by manage\n", message.mdata);
					kill(message.mdata, SIGINT);
				}else{
					printf("sending pid to compute process\n");
					msgsnd(msqid, &message, sizeof(message.mdata), 0);	//send index back to compute process
				}
				break;
			case 2:
				isNew = 1;
				for (int i = 0; i < 20; ++i){
					if (shmaddr->perfs[i]==message.mdata){
						isNew=0;
					}
				}
				if (isNew){
					if (!addPerfToTable(message.mdata)){
						printf("Error adding perf to table. Perf table is full.\n");
					}
				}
				int i = 0;
				
				break;
			default:
				printf("Manage read weird message of type: %d with data %lu\n", message.mdata,message.mtype);
				break;
		}
	}	

	//detach shared memory
	shmdt(shmaddr);
	shmctl(shmid, IPC_RMID, NULL);	//unlink shared memory so it is destroyed

	//Close message queue
	msgctl(msqid, IPC_RMID, NULL);

	return 0;
}

int addPerfToTable(int val){
	int i = 0;
	while(i < 20){
		if (shmaddr->perfs[i] == 0){
			shmaddr->perfs[i] = val;
			return 1;
		}
		i++;
	}
	return -1;
}


int addPidToTable(int pid){	//return -1 if process table is full, terminates compute process
	//loop through shmadder

	process * proc;
	for (int i = 0; i < 20; ++i){
		if (shmaddr->processes[i].pid == 0){
			proc = &(shmaddr->processes[i]);
			proc->pid = pid;
			proc->tested = 0;
			proc->skipped = 0;
			proc->found = 0;
			//insert into pid table
			return i;
		}
	}
	return -1;
}

void die(int signum){
	//block incoming signals, iterrupt all compute programs, sleep, cleanup, die

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	printf("Manage received signal %d. Cleaning and quitting...\n", signum);
	/*interrupt compute programs*/
	int comppid;
	for (int i = 0; i < 20; ++i){
		if ((comppid = shmaddr->processes[i].pid) != 0){
			kill(comppid, SIGINT);
		}
	}

	/*sleep*/
	sleep(5);

	/* Cleanup */
	shmdt(shmaddr);					//detach shared memory
	shmctl(shmid, IPC_RMID, NULL);	//unlink shared memory so it is destroyed
	msgctl(msqid, IPC_RMID, NULL); 	//Close message queue

	printf("Memory cleared. Manage quitting\n");
	exit(0);	//die

}






