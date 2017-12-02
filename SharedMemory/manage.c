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
}shm_buf;

shm_buf * shmaddr;		//global address for shared memory

int main(int argc, char *argv[]){
	if (argc != 1){
		printf("%s",useage);
		return 0;
	}
	int key = 60302;

	//create or find + link shared 
	//shm_buf * shmaddr;	//address of shared memory
	int shmid;		//shared memory id
	long shmsize = sizeof(shm_buf);	//size of shared memory
	shmsize = 400;
	printf("going to get shared memory of size: %lu\n", shmsize);
	if ((shmid = shmget(key, shmsize, IPC_CREAT | 0666)) < 0){	//find or create shared memory
		printf("got shared memory id: %d\n", shmid);
		printf("Error getting shared memory\n");
		exit(1);
	}

	if ((shmaddr = shmat(shmid, NULL, 0)) == (int *) -1){	//attach shared memory at next open address for reading and writing
		exit(1);
	}


	//Listen for messages from compute processes
	//	when a compute process begins, and when perfect numbers is updated

	//Open Message Queue
		//*msgsize = 30*//
	key_t msgkey;
	int flags = IPC_CREAT;
	int msqid;

	msgkey = ftok(".", 'j');

	if ((msqid = msgget(key, flags | 0666)) == -1){
		printf("Error opening message queue in Compute\n");
		exit(1);
	}

	//Recieve messages
	msgbuf message;
	while(1){
		printf("Manage going to wait for messages\n");
		if ((msgrcv(msqid, &message, sizeof(message.mdata), -2, 0)) < 0){	//receive only type 1 and type 2 messages (pids and perfs)
			printf("Error recieving message. Errno: %d\n", errno);
			break;
		}
		printf("Processing message: %d of type %lu\n", message.mdata, message.mtype);
		switch(message.mtype){
			case 1:
				if (addPidToTable(message.mdata) == -1){
					printf("compute process %d exceeds process table. Killed by manage\n", message.mdata);
					kill(message.mdata, SIGINT);
				}
				break;
			case 2:
				printf("got a new perfect\n");
				break;
			default:
				printf("Manage read weird message of type: %d with data %d\n", message.mdata,message.mtype);
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

int addPidToTable(int pid){	//return -1 if process table is full, terminates compute process
	printf("adding pid %d to table\n", pid);
	return -1;
}








