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
#define MSQ_PAYLOAD_LEN 32
#define SHMSIZE 4096

/***
 * Signal Handlers
 */
void chld_handler(int sig);

void waitForChildProcess(pid_t pid, char *name);
char *int2str(int n);

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

	pid_t mandelCalcPID = fork();
	if (mandelCalcPID == 0) {
		char * mcargs[] = {MANDEL_CALC_EXECUTABLE, shmidstr, mid1str, NULL};

		/* setup pipe (parent --> child.stdin) */
		dup2(fd_a[0], STDIN);
		close(fd_a[0]);

		/* setup pipe (child.stdout --> fd_b) */
		dup2(fd_b[1], STDOUT);
		close(fd_b[1]);

		return (execvp(MANDEL_CALC_EXECUTABLE, mcargs) == -1) ? -1 : 1;
	} else {
		pid_t mandelDisplayPID = fork();
		if (mandelDisplayPID == 0) {
			char * mdargs[] = {MANDEL_DISPLAY_EXECUTABLE, shmidstr, mid1str, mid2str, NULL};

			/* setup pipe (fd_b --> child.stdin) */
			dup2(fd_b[0], STDIN);
			close(fd_b[0]);

			return (execvp(MANDEL_DISPLAY_EXECUTABLE, mdargs) == -1) ? -2 : 2;
		} else {
			/* close unused pipe ends */
			close(fd_a[0]);
			close(fd_b[0]);
			close(fd_b[1]);

			msgbuf_t mbuf;
			initMsgBuf(&mbuf, "testfilename.txt");
			msgsnd(msqid2, &mbuf, strlen(mbuf.mtext), 0);

			bzero(mbuf.mtext, 32);
			msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);
			printf("%s\n", mbuf.mtext);

			bzero(mbuf.mtext, 32);
			msgrcv(msqid1, &mbuf, sizeof(mbuf.mtext), 0, 0);
			printf("%s\n", mbuf.mtext);

			waitForChildProcess(mandelCalcPID, "MandelCalc");
			waitForChildProcess(mandelDisplayPID, "MandelDisplay");

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

	if (WIFEXITED(status))
		printf("%s exited with status %d\n", name, WEXITSTATUS(status));
	else if (WIFSIGNALED(status)) {
		printf("%s received signal %d\n", name, WTERMSIG(status));
		if (WCOREDUMP(status))
			printf("core dumped\n");
	} else if (WIFSTOPPED(status))
		printf("%s was stopped by signal %d\n", name, WSTOPSIG(status));
	else
		printf("%s exited for unknown reason???\n", name);
}

char *int2str(int n) {
	char *buf = (char *) malloc(32);
	sprintf(buf, "%d", n);
	return buf;
}


void chld_handler(int sig) {

}
