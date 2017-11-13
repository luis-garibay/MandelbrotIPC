#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define MANDEL_CALC_EXECUTABLE "./mandelCalc"
#define MANDEL_DISPLAY_EXECUTABLE "./mandelDisplay"

void reportExitStatus(char *name, int status);

int main(int argc, const char *argv[]) {
	if (argc != 3) {
		printf("usage: %s mandel-calc-arg mandel-display-arg\n", argv[0]);
		exit(-1);
	}

	pid_t mandelCalcPID, mandelDisplayPID;
	int mandelCalcStatus, mandelDisplayStatus;

	int fd_a[2], fd_b[2];

	pipe(fd_a);
	pipe(fd_b);

	mandelCalcPID = fork();
	if (mandelCalcPID == 0) {
		char * mcargs[] = {MANDEL_CALC_EXECUTABLE, argv[1], NULL};
		return (execvp(MANDEL_CALC_EXECUTABLE, mcargs) == -1) ? -1 : 1;
	} else {
		mandelDisplayPID = fork();
		if (mandelDisplayPID == 0) {
			char * mdargs[] = {MANDEL_DISPLAY_EXECUTABLE, argv[2], NULL};
			return (execvp(MANDEL_DISPLAY_EXECUTABLE, mdargs) == -1) ? -2 : 2;
		} else {
			wait4(mandelDisplayPID, &mandelDisplayStatus, 0, NULL);
			wait4(mandelCalcPID, &mandelCalcStatus, 0, NULL);

			reportExitStatus("MandelCalc", mandelCalcStatus);
			reportExitStatus("MandelDisplay", mandelDisplayStatus);
		}
	}

	return 0;
}

void reportExitStatus(char *name, int status) {
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
