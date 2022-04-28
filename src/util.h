#ifndef UTIL_H
#define UTIL_H

#include "my_stdio.h"

#define BUFF_SIZE (4096)
#define NO_OP (0)
#define READ_LAST (1)
#define WRITE_LAST (2)

struct _so_file {
	char *buf;      // IO buffer
	int buf_cursor; // indicates the start of valid data in the buffer
	int buf_end;    // indicates the end of valid data in the buffer
	int fd;         // File descriptor
	int error;      // 0 if no error occured or -1 otherwise
	int eof;        // 0 if not reached yet or 1 if reached
	int last_op;    // SO_READ_LAST or SO_WRITE_LAST
	int child_pid;	// PID of child process created with so_popen
};

int open_file(const char *pathname, const char *mode);
long refill_buffer(SO_FILE *stream);
void reset_buffer(SO_FILE *stream);
long copy_from_buf(SO_FILE *stream, void *ptr, size_t bytes);

#endif // UTIL_H
