#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifndef MESSAGING_H
#define MESSAGING_H

/***
 * Pipes
 */

void readFromPipe(int file, char *buf) {
	FILE *stream = fdopen(file, "r");
	int c, i = 0;
	while ((c = fgetc(stream)) != '\n') {
		buf[i] = c;
		++i;
	}
	buf[i] = '\0';
}

void writeToPipe(int file, char *buf) {
	FILE *stream = fdopen(file, "w");
	fprintf(stream, "%s", buf);
	fflush(stream);
}

/***
 * Message Queue
 */

#define MSGQ_PAYLOAD_LEN 32

typedef struct {
	long mtype;
	char mtext[MSGQ_PAYLOAD_LEN];
} msgbuf_t;

void initMsgBuf(msgbuf_t *buf, const char *cstr) {
	buf->mtype = 1;
	bzero(buf->mtext, MSGQ_PAYLOAD_LEN);
	strncpy(buf->mtext, cstr, MSGQ_PAYLOAD_LEN-1);
}

/***
 * Shared Memory
 */

#define SHMSIZE 16384

#endif
