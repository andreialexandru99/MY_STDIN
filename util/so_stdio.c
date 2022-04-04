#include "so_stdio.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct _so_file {
    int fd;
    char buf[SO_BUFF_SIZE];
    long cursor;
};

SO_FILE *so_fopen(const char *pathname, const char *mode) {
    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (file == NULL) return NULL;
    memset(file->buf, 0, SO_BUFF_SIZE);
    file->cursor = 0; // TODO: Check if cursor is set correctly?
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
        free(file);
        return NULL;
    }
    if (file->fd < 0) {
        // TODO: Handle failure to open file!
    }

    return file;
}

int so_fclose(SO_FILE *stream) {
    int ret = close(stream->fd);
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
    return -1;
}

long so_ftell(SO_FILE *stream) {
    return -1;
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
    return -1;
}

int so_ferror(SO_FILE *stream) {
    return -1;
}

SO_FILE *so_popen(const char *command, const char *type) {
    return NULL;
}

int so_pclose(SO_FILE *stream) {
    return -1;
}