#ifndef SO_IO_H
#define SO_IO_H

#include "so_stdio.h"

#define SO_BUFF_SIZE (4096)
#define SO_READ_LAST (1)
#define SO_WRITE_LAST (2)

struct _so_file {
	char *buf;      // IO buffer
	int buf_cursor; // indicates the start of valid data in the buffer
	int buf_end;    // indicates the end of valid data in the buffer
	int fd;         // File descriptor
	int error;      // 0 if no error occured or -1 otherwise
	int eof;        // 0 if not reached yet or 1 if reached
	int last_op;    // SO_READ_LAST or SO_WRITE_LAST
};

long refill_buffer(SO_FILE *stream);

void reset_buffer(SO_FILE *stream);

#endif // SO_IO_H
