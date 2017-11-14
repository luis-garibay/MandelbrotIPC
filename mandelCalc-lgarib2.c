#include <stdio.h>
#include <stdlib.h>

#define STDIN 0

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("MandelCalc: no argument given\n");
		exit(-1);
	}

	printf("MandelCalc: argument = %s\n", argv[1]);

	// DEBUG: test stdin
	char buf[32];
	read(STDIN, buf, 32);
	printf("%s\n", buf);

	return 0;
}
