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



char *useage = "Usage: ./manage\n";

typedef struct _msg{
	long mtype;
	char mtext[30];
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

int main(int argc, char *argv[]){
	if (argc != 1){
		printf("%s",useage);
		return 0;
	}
	int key = 60302;

	//create or find + link shared 
	process * shmaddr;	//address of shared memory
	int shmid;		//shared memory id
	long shmsize = sizeof(shm_buf);	//size of shared memory
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

	if ((msqid = msgget(key, flags)) == -1){
		printf("Error opening message queue in Compute\n");
		exit(1);
	}

	//Recieve messages
	msgbuf * message;
	message = calloc(1, sizeof(msgbuf));
	message->mtype = 0;	//set message type to zero to always get first value of msg queue

	while(1){
		printf("Manage going to wait for messages\n");
		msgrcv(msqid, message, sizeof(message->mtext), 0, 0);
		printf("Processing message: %s\n", message->mtext);
	}	

	//detach shared memory
	shmdt(shmaddr);
	shmctl(shmid, IPC_RMID, NULL);	//unlink shared memory so it is destroyed

	//Close message queue
	msgctl(msqid, IPC_RMID, NULL);

	return 0;
}












