#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("MandelCalc: no argument given\n");
		exit(-1);
	}

	printf("MandelCalc: argument = %s\n", argv[1]);

	return 0;
}
