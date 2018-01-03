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
#define STDOUT 1

void usr1_handler(int sig);

int numLoops;

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("MandelCalc: not enough arguments\n");
		exit(-1);
	}

	int shmid = atoi(argv[1]),
		msqid1 = atoi(argv[2]);
	
	int *shmaddr = shmat(shmid, NULL, 0);

	signal(SIGUSR1, usr1_handler);

	float xMin, xMax, yMin, yMax;
	int nRows, nCols, maxIter;

	numLoops = 0;
	while (1) {
		/* read input data from stdin */
		scanf("%d,%d,%f,%f,%f,%f,%d", &nRows, &nCols, &xMin, &xMax, &yMin, &yMax, &maxIter);

		/* implement mandelbrot algorithm to fill shared memory */
		float deltaX = (xMax - xMin)/(nCols - 1),
			deltaY = (yMax - yMin)/(nRows - 1);

		int r, c, n;
		for (r = 0; r < nRows; ++r) {
			float Cy = yMin + r * deltaY;
			for (c = 0; c < nCols; ++c) {
				float Cx = xMin + c * deltaX,
						Zx=0.0, Zy=0.0;
				for (n = 0; n < maxIter; ++n) {
					if (Zx * Zx + Zy * Zy >= 4.0)
						break;

					float Zx_next = Zx*Zx - Zy*Zy + Cx,
						Zy_next = 2.0*Zx*Zy + Cy;
					Zx = Zx_next;
					Zy = Zy_next;
				}

				if (n >= maxIter)
					*(shmaddr + r*nCols + c) = -1; // store -1 in data[r][c]
				else
					*(shmaddr + r*nCols + c) = n; // store n in data[r][c]
			}
		}

		/* write modified data to stdout */
		char buf[256];
		sprintf(buf, "%d,%d,%f,%f,%f,%f,%d\n", nRows, nCols, xMin, xMax, yMin, yMax, maxIter);
		writeToPipe(STDOUT, buf);

		/* write done message to message queue 1 */
		msgbuf_t mbuf;
		initMsgBuf(&mbuf, "done");
		if (msgsnd(msqid1, &mbuf, strlen(mbuf.mtext), 0) == -1)
			exit(-1);

		++numLoops;
	}

	return 0;
}

void usr1_handler(int sig) {
	/* exit w/ # of images calculated */
	int numImagesCalculated = numLoops;
	exit(numImagesCalculated);
}
