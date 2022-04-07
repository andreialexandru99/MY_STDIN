#ifndef SO_IO_H
#define SO_IO_H

#include "so_stdio.h"

#define SO_BUFF_SIZE (4096)
#define SO_READ_LAST (1)
#define SO_WRITE_LAST (2)

struct _so_file {
    int fd;
    char *buf;
    int buf_cursor;
    int buf_end;
    int error;
    int eof;
    int last_op; // SO_READ_LAST or SO_WRITE_LAST
    // TODO: ADD CURSOR
};

long so_read(int fd, void *buf, size_t nbytes);

long refill_buffer(SO_FILE *stream);

long so_write(int fd, void *buf, size_t nbytes);

void reset_buffer(SO_FILE *stream);

#endif // SO_IO_H