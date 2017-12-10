
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char const *argv[])
{
	
	//get directory
	DIR * dirp;
	char cwd[256];
	getcwd(cwd, 256);
	if (errno){
		perror("get cwd");
	}
	printf("got cwd: %s\n", cwd);
	dirp = opendir(cwd);

	int fd[2];
	pipe(fd);

	struct dirent * dent;
	char * buff;

	while((dent = readdir(dirp)) != NULL){

		if (dent->d_type != DT_REG){
			printf("%s not a regular file\n", dent->d_name);
			continue;
		}
		buff = dent->d_name;
		strcat(buff, "\n");
		if ((write(fd[1], buff, strlen(buff))) == -1){
			perror("writing filename");
			break;
		}
	}

	closedir(dirp);

	int pid = fork();
	switch(pid){
		case 0:	//child
			dup2(fd[0], 0);	//assing read to stdin
			close(fd[0]);
			close(fd[1]);
			printf("child execing\n");
			execv("/usr/bin/sort", NULL);
			break;
		default:
			close(fd[0]);
			close(fd[1]);
			printf("parent waiting\n");
			waitpid(pid, NULL, 0);

	}


	return 0;
}