#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "messaging.h"

#define STDIN 0

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("MandelDisplay: no argument given\n");
		exit(-1);
	}

	/* setup message queues */
	int shmid = atoi(argv[1]),
		msqid1 = atoi(argv[2]),
		msqid2 = atoi(argv[3]);

	msgbuf_t buf;
	bzero(buf.mtext, 32);
	msgrcv(msqid2, &buf, sizeof(buf.mtext), 0, 0);
	printf("filename: %s\n", buf.mtext);

	initMsgBuf(&buf, "child 2 done");
	msgsnd(msqid1, &buf, strlen(buf.mtext), 0);

	char *shmaddr = shmat(shmid, NULL, 0);
	printf("%s\n", shmaddr);
	
	return 0;
}
