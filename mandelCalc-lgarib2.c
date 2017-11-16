#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "messaging.h"

#define STDIN 0
#define STDOUT 1

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("MandelCalc: no argument given\n");
		exit(-1);
	}

	int shmid = atoi(argv[1]),
		msqid1 = atoi(argv[2]);

	msgbuf_t buf;
	initMsgBuf(&buf, "child 1 done");
	msgsnd(msqid1, &buf, strlen(buf.mtext), 0);

	char *shmaddr = shmat(shmid, NULL, 0);
	bzero(shmaddr, 32);
	strcpy(shmaddr, "child in shared mem");

	return 0;
}
