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

typedef enum {
	false = 0,
	true
} Boolean;

typedef struct {
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	int nRows;
	int nCols;
	int maxIter;
	char filename[1024];
} MandelbrotData;

/***
 * Signal Handlers
 */
void chld_handler(int sig);

void waitForChildProcess(pid_t pid, char *name);
char *int2str(int n);
Boolean userWantsToSolveAnotherProblem();
void getMandelbrotInputFromUser(MandelbrotData *mbData);
void sendMandelbrotDataThroughPipe(MandelbrotData *mbData, int outPipe);
void flushStdin() {
	char c=0;
	while ((c = (char)getchar()) != '\n' && c != EOF);
}

pid_t mandelCalcPID, mandelDisplayPID;

int main(int argc, const char *argv[]) {
	/* Create Pipes */
	int parentToChild1Pipe[2], child1ToChild2Pipe[2];
	pipe(parentToChild1Pipe);
	pipe(child1ToChild2Pipe);

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

	/****
	 * Fork Off Child 1
	 */
	mandelCalcPID = fork();
	if (mandelCalcPID == 0) {
		char *mcargs[] = {MANDEL_CALC_EXECUTABLE, shmidstr, mid1str, NULL};

		/* setup pipe (parent pipe end --> stdin of child 1) */
		dup2(parentToChild1Pipe[0], STDIN);
		close(parentToChild1Pipe[0]);

		/* setup pipe (stdout of child 1 --> stdin of child 2) */
		dup2(child1ToChild2Pipe[1], STDOUT);
		close(child1ToChild2Pipe[1]);

		execvp(MANDEL_CALC_EXECUTABLE, mcargs);

		exit(0);
	}
	
	/****
	 * Fork Off Child 2
	 */
	mandelDisplayPID = fork();
	if (mandelDisplayPID == 0) {
		char *mdargs[] = {MANDEL_DISPLAY_EXECUTABLE, shmidstr, mid1str, mid2str, NULL};

		/* setup pipe (stdout of child 1 --> stdin of child 2) */
		dup2(child1ToChild2Pipe[0], STDIN);
		close(child1ToChild2Pipe[0]);

		execvp(MANDEL_DISPLAY_EXECUTABLE, mdargs);

		exit(0);
	}

	/* close unused pipe ends */
	close(parentToChild1Pipe[0]);
	close(child1ToChild2Pipe[0]);
	close(child1ToChild2Pipe[1]);

	while (1) {
		MandelbrotData mbData;
		
		getMandelbrotInputFromUser(&mbData);
		
		/* write filename to message queue 2 */
		msgbuf_t mbuf;
		initMsgBuf(&mbuf, mbData.filename);
		msgsnd(msqid2, &mbuf, strlen(mbuf.mtext), 0);

		sendMandelbrotDataThroughPipe(&mbData, parentToChild1Pipe[1]);

		/* listen for done messages from both children */
		msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);
		msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);

		if (!userWantsToSolveAnotherProblem()) {
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

Boolean userWantsToSolveAnotherProblem() {
	printf("Do you want to solve another problem (y/n)?\n");
	char userInput = getchar();
	flushStdin();
	if (userInput == 'y')
		return true;
	else
		return false;
}

void getMandelbrotInputFromUser(MandelbrotData *mbData) {
	printf("enter data:\n");
	scanf("%d %d %f %f %f %f %d %s", &mbData->nRows, &mbData->nCols, &mbData->xMin, &mbData->xMax, &mbData->yMin, &mbData->yMax, &mbData->maxIter, mbData->filename);
	flushStdin();
}

void sendMandelbrotDataThroughPipe(MandelbrotData *mbData, int outPipe) {
	char buf[256];
	sprintf(buf, "%d,%d,%f,%f,%f,%f,%d\n", mbData->nRows, mbData->nCols, mbData->xMin, mbData->xMax, mbData->yMin, mbData->yMax, mbData->maxIter);
	writeToPipe(outPipe, buf);
}


void chld_handler(int sig) {
	waitForChildProcess(mandelCalcPID, "MandelCalc");
	waitForChildProcess(mandelDisplayPID, "MandelDisplay");
}
