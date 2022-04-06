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
    int error;
    int eof;
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

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    return -1;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    return -1;
}

int so_fgetc(SO_FILE *stream) {
    return -1;
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