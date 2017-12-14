#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

typedef struct _shared{
	int number;
	char text[200];
}shbuf;

typedef struct _jkmsg{
	long mtype;
	char mtext[200];
}jk_buf;
int main(int argc, char const *argv[]){
	jk_buf buf; int msqid; key_t key;
	shbuf *shaddr;int shid;
	if((key=ftok(".", 'J'))==-1){
		perror("ftok"); exit(1);}
	if((msqid = msgget(key, 0666 | IPC_CREAT)) == -1){
		perror("msgget"); exit(1);}
	if((shid=shmget(key,sizeof(shbuf),IPC_CREAT|0666))<0){perror("shmget");}
	if ((shaddr=shmat(shid,NULL,0))==NULL){perror("shmat");}
	printf("Enter text to send. ^D to quit\n");
	buf.mtype = 1;
	while(fgets(shaddr->text,sizeof(shaddr->text),stdin)!=NULL){
		printf("did fgets on :%s\n", shaddr->text);
		int len = strlen(shaddr->text); shaddr->number++;
		if(shaddr->text[len-1]=='\n') shaddr->text[len-1]='\0';//remove \n
		strncpy(buf.mtext,shaddr->text,len+1);
		if(msgsnd(msqid,&buf,len+1,IPC_NOWAIT)==-1) perror("msgsnd");
	}
	if (msgctl(msqid,IPC_RMID,NULL)==-1){perror("msgctl");exit(1);}
	shmdt(shaddr); shmctl(shid,IPC_RMID,NULL);
	return 0;
}