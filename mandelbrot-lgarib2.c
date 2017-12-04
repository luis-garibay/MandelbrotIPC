#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "messaging.h"

#define STDIN 0
#define STDOUT 1
#define MANDEL_CALC_EXECUTABLE "./mandelCalc"
#define MANDEL_DISPLAY_EXECUTABLE "./mandelDisplay"

/***
 * Signal Handlers
 */
void chld_handler(int sig);

void waitForChildProcess(pid_t pid, char *name);
char *int2str(int n);
void flushStdin() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

pid_t mandelCalcPID, mandelDisplayPID;

int main(int argc, const char *argv[]) {
	/* Create Pipes */
	int fd_a[2], fd_b[2];
	pipe(fd_a);
	pipe(fd_b);

	/* Create Message Queues */
	int msqid1 = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
	int msqid2 = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
	char *mid1str = int2str(msqid1), *mid2str = int2str(msqid2);

	/* Create Shared Memory */
	int shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | 0600);
	char *shmaddr = shmat(shmid, NULL, 0);
	char *shmidstr = int2str(shmid);

	/* Setup Signal Handlers */
	signal(SIGCHLD, chld_handler);

	mandelCalcPID = fork();
	if (mandelCalcPID == 0) {
		char * mcargs[] = {MANDEL_CALC_EXECUTABLE, shmidstr, mid1str, NULL};

		/* setup pipe (parent --> child.stdin) */
		dup2(fd_a[0], STDIN);
		close(fd_a[0]);

		/* setup pipe (child.stdout --> fd_b) */
		dup2(fd_b[1], STDOUT);
		close(fd_b[1]);

		execvp(MANDEL_CALC_EXECUTABLE, mcargs);

		exit(0);
	} else {
		mandelDisplayPID = fork();
		if (mandelDisplayPID == 0) {
			char * mdargs[] = {MANDEL_DISPLAY_EXECUTABLE, shmidstr, mid1str, mid2str, NULL};

			/* setup pipe (fd_b --> child.stdin) */
			dup2(fd_b[0], STDIN);
			close(fd_b[0]);

			execvp(MANDEL_DISPLAY_EXECUTABLE, mdargs);

			exit(0);
		} else {
			/* close unused pipe ends */
			close(fd_a[0]);
			close(fd_b[0]);
			close(fd_b[1]);

			while (1) {
				char filename[1024];
				float xMin, xMax, yMin, yMax;
				int nRows, nCols, maxIter;
				/* read problem info from keyboard */
				printf("enter data:\n");
				scanf("%d,%d,%f,%f,%f,%f,%d,%s", &nRows, &nCols, &xMin, &xMax, &yMin, &yMax, &maxIter, filename);
				flushStdin();
				
				/* write filename to message queue 2 */
				msgbuf_t mbuf;
				initMsgBuf(&mbuf, filename);
				msgsnd(msqid2, &mbuf, strlen(mbuf.mtext), 0);

				/* write xmin, xmax, ymin, ymax, nrows, ncols, and maxiter to pipe */
				char buf[256];
				sprintf(buf, "%d,%d,%f,%f,%f,%f,%d\n", nRows, nCols, xMin, xMax, yMin, yMax, maxIter);
				writeToPipe(fd_a[1], buf);

				/* listen for done messages from both children */
				msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);
				msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);

				/* prompt user */
				printf("Do you want to solve another problem (y/n)?\n");
				char userInput = getchar();
				flushStdin();
				if (userInput != 'y') {
					/* send SIGUSR1 signals to both children */
					kill(mandelCalcPID, SIGUSR1);
					kill(mandelDisplayPID, SIGUSR1);

					break;
				}
			}

			/* wait for both children and report exit codes */
			wait4(mandelCalcPID, NULL, 0, NULL);
			wait4(mandelDisplayPID, NULL, 0, NULL);

			/* destroy shared memory */
			shmdt(shmaddr);
			shmctl(shmid, IPC_RMID, NULL);

			/* destroy message queue */
			msgctl(msqid1, IPC_RMID, NULL);
			msgctl(msqid2, IPC_RMID, NULL);

			free(mid1str);
			free(mid2str);
		}
	}

	

	return 0;
}

void waitForChildProcess(pid_t pid, char *name) {
	int status;

	wait4(pid, &status, 0, NULL);

	printf("%s exited with status %d\n", name, WEXITSTATUS(status));
}

char *int2str(int n) {
	char *buf = (char *) malloc(32);
	sprintf(buf, "%d", n);
	return buf;
}


void chld_handler(int sig) {
	waitForChildProcess(mandelCalcPID, "MandelCalc");
	waitForChildProcess(mandelDisplayPID, "MandelDisplay");
}
