#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define STDIN 0
#define STDOUT 1
#define MANDEL_CALC_EXECUTABLE "./mandelCalc"
#define MANDEL_DISPLAY_EXECUTABLE "./mandelDisplay"

void waitForChildProcess(pid_t pid, char *name);

int main(int argc, const char *argv[]) {
	if (argc != 3) {
		printf("usage: %s mandel-calc-arg mandel-display-arg\n", argv[0]);
		exit(-1);
	}

	pid_t mandelCalcPID, mandelDisplayPID;

	/* Create Pipes */
	int fd_a[2], fd_b[2];
	pipe(fd_a);
	pipe(fd_b);

	mandelCalcPID = fork();
	if (mandelCalcPID == 0) {
		char * mcargs[] = {MANDEL_CALC_EXECUTABLE, argv[1], NULL};

		/* setup pipe (parent --> child.stdin) */
		dup2(fd_a[0], STDIN);
		close(fd_a[0]);

		/* setup pipe (child.stdout --> fd_b) */
//		dup2(fd_b[1], STDOUT);
//		close(fd_b[1]);

		return (execvp(MANDEL_CALC_EXECUTABLE, mcargs) == -1) ? -1 : 1;
	} else {
		mandelDisplayPID = fork();
		if (mandelDisplayPID == 0) {
			char * mdargs[] = {MANDEL_DISPLAY_EXECUTABLE, argv[2], NULL};

			/* setup pipe (fd_b --> child.stdin) */
			dup2(fd_b[0], STDIN);
			close(fd_b[0]);

			return (execvp(MANDEL_DISPLAY_EXECUTABLE, mdargs) == -1) ? -2 : 2;
		} else {
			/* close unused pipe ends */
			close(fd_a[0]);
			close(fd_b[0]);
			close(fd_b[1]);

			write(fd_a[1], "this is message one", 20);

			waitForChildProcess(mandelCalcPID, "MandelCalc");
			waitForChildProcess(mandelDisplayPID, "MandelDisplay");
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
