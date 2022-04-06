#include "so_stdio.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

SO_FILE *so_fopen(const char *pathname, const char *mode) {
    // Allocate memory for SO_FILE structure
    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (file == NULL) return NULL;

    // Allocate memory for IO buffer
    file->buf = calloc(SO_BUFF_SIZE, sizeof(char));
    if (file->buf == NULL) {
        free(file);
        return NULL;
    }

    // Initialize error with 0 indicating no error has been encountered yet
    file->error = 0;
    // Initialize eof with 0 indicating the end of the file has not been reached yet
    file->eof = 0;
    // Initialize buf_cursor, buf_end and last_op to zero
    file->buf_cursor = 0;
    file->buf_end = 0;
    file->last_op = 0;

    // Open file
    if (strcmp(mode, "r") == 0) {
        file->fd = open(pathname, O_RDONLY);
    }
    else if (strcmp(mode, "r+") == 0) {
        file->fd = open(pathname, O_RDWR);
    }
    else if (strcmp(mode, "w") == 0) {
        file->fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    }
    else if (strcmp(mode, "w+") == 0) {
        file->fd = open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0644);
    }
    else if (strcmp(mode, "a") == 0) {
        file->fd = open(pathname, O_APPEND | O_CREAT, 0644);
    }
    else if (strcmp(mode, "a+") == 0) {
        file->fd = open(pathname, O_RDWR | O_APPEND | O_CREAT, 0644);
    } else {
        // File mode unknown
        free(file->buf);
        free(file);
        return NULL;
    }

    // Check if file was opened successfully
    if (file->fd < 0) {
        free(file->buf);
        free(file);
        return NULL;
    }
    return file;
}

int so_fclose(SO_FILE *stream) {
    int ret = close(stream->fd);
    free(stream->buf);
    free(stream);
    if (ret < 0) return SO_EOF;
    return 0;
}

int so_fileno(SO_FILE *stream) {
    return stream->fd;
}

int so_fflush(SO_FILE *stream) {
    return -1;
}

int so_fseek(SO_FILE *stream, long offset, int whence) {
    long pos = lseek(stream->fd, offset, whence);
    if (pos < 0) {
        stream->error = errno;
        return -1;
    }
    return 0;
}

long so_ftell(SO_FILE *stream) {
    long pos = lseek(stream->fd, 0, SEEK_CUR);
    if (pos < 0) stream->error = errno;
    return pos;
}

int refill_buffer(SO_FILE *stream) {
    int bytes_read;
    memset(stream->buf, 0, SO_BUFF_SIZE); // TODO: IS THIS NEEDED?
    bytes_read = read(stream->fd, stream->buf, SO_BUFF_SIZE);
    if (bytes_read < 0) {
        stream->error = errno;
        return -1;
    }
    if (bytes_read == 0) {
        stream->eof = 1;
        return 0;
    }
    stream->buf_cursor = 0;
    stream->buf_end = bytes_read;
    return 1;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    int bytes_available, bytes_copied = 0, bytes_left = nmemb, bytes_to_copy, ret;
    if (stream->last_op == SO_WRITE_LAST) {
        // TODO: flush the buffer
    }
    stream->last_op = SO_READ_LAST;

    while (bytes_copied < nmemb) {
        // Check if buffer refill is needed
        bytes_available = stream->buf_end - stream->buf_cursor;
        if (bytes_available == 0) {
            ret = refill_buffer(stream);
            if (ret < 0) {
                return 0;
            }
            if (ret == 0) {
                break;
            }
        }
        bytes_available = stream->buf_end - stream->buf_cursor;
        // Copy from buffer
        bytes_left = nmemb - bytes_copied;
        bytes_to_copy = bytes_available < bytes_left ? bytes_available : bytes_left;
        memcpy(((char *)ptr) + bytes_copied, stream->buf, bytes_to_copy);
        bytes_copied += bytes_to_copy;
        stream->buf_cursor += bytes_to_copy;
    }
    return bytes_copied;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    return -1;
}

int so_fgetc(SO_FILE *stream) {
    if (stream->last_op == SO_WRITE_LAST) {
        // TODO: flush the buffer
    }
    stream->last_op = SO_READ_LAST;

    // Check if data is available in the buffer
    if (stream->buf_end - stream->buf_cursor > 0) {
        return (int)stream->buf[stream->buf_cursor++];
    }
    else {
        // Refill buffer
        if (refill_buffer(stream) <= 0) {
            return SO_EOF;
        }
    }
    return (int)stream->buf[stream->buf_cursor++];
}

int so_fputc(int c, SO_FILE *stream) {
    return -1;
}

int so_feof(SO_FILE *stream) {
    return stream->eof;
}

int so_ferror(SO_FILE *stream) {
    return stream->error;
}

SO_FILE *so_popen(const char *command, const char *type) {
    return NULL;
}

int so_pclose(SO_FILE *stream) {
    return -1;
}