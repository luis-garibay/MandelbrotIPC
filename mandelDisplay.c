#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "messaging.h"

#define STDIN 0

void usr1_handler(int sig);

int numLoops;

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("MandelDisplay: no argument given\n");
		exit(-1);
	}

	/* setup message queues */
	int shmid = atoi(argv[1]),
		msqid1 = atoi(argv[2]),
		msqid2 = atoi(argv[3]);
	
	int *shmaddr = shmat(shmid, NULL, 0);
	
	msgbuf_t mbuf;

	signal(SIGUSR1, usr1_handler);

	int nRows, nCols, maxIter;
	float xMin, xMax, yMin, yMax;

	numLoops = 0;
	while (1) {
		/* read data from stdin */
		scanf("%d,%d,%f,%f,%f,%f,%d", &nRows, &nCols, &xMin, &xMax, &yMin, &yMax, &maxIter);

		/* read filename from message queue 2 */
		bzero(mbuf.mtext, MSGQ_PAYLOAD_LEN);
		if (msgrcv(msqid2, &mbuf, sizeof(mbuf.mtext), 0, 0) == -1)
			exit(-1);

		/* open the file */
		FILE *fp = fopen(mbuf.mtext, "w+");

		/* read data from shared memory and display image on screen */
		int r, c, n,
			nColors = 15;
		char colors[] = ".-~:+*%O8&?$@#X";
		for (r = nRows-1; r >= 0; --r) {
			for (c = 0; c < nCols-1; ++c) {
				n = *(shmaddr + r*nCols+c);

				if (n < 0)
					printf(" ");
				else
					printf("%c", colors[n % nColors]);
			}
			printf("\n");
		}

		if (fp != NULL) {
			/* save data to a data file */
			for (r = nRows-1; r >= 0; --r) {
				for (c = 0; c < nCols-1; ++c)
					fprintf(fp, "%d ", *(shmaddr + r*nCols + c));

				fprintf(fp, "\n");
			}

			/* close file when all data has been written */
			fclose(fp);
		}
			
		/* write done message to message queue 1 */
		initMsgBuf(&mbuf, "c2 done");
		if (msgsnd(msqid1, &mbuf, strlen(mbuf.mtext), 0) == -1)
			exit(-2);

		++numLoops;
	}

	return 0;
}


void usr1_handler(int sig) {
	/* exit code equal to number of images calculated */
	int numImagesCalculated = numLoops;
	exit(numImagesCalculated);
}
