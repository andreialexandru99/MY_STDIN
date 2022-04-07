#include "so_io.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/**
 * @brief Opens file from pathname in given mode
 * @return Pointer to SO_FILE structure associated or NULL if an error occurred
 */
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

/**
 * @brief Closes given stream and frees all memory associated with the it
 * @return 0 if successful or SO_EOF if an error occurred
 */
int so_fclose(SO_FILE *stream) {
    int ret = close(stream->fd);
    free(stream->buf);
    free(stream);
    if (ret < 0) return SO_EOF;
    return 0;
}

/**
 * @return The file descriptor if the stream 
 */
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

/**
 * @brief Reads from fd into buf until nbytes or EOF reached
 * @return Bytes read or a negative value if an error occurred
 */
int so_read(int fd, void *buf, size_t nbytes) {
    int bytes_read = 0, ret;

    while (bytes_read < nbytes) {
        ret = read(fd, ((char *)buf) + bytes_read, nbytes - bytes_read);
        if (ret < 0) return ret;
        if (ret == 0) break;
        bytes_read += ret;
    }
    return bytes_read;
}

/**
 * @brief Refills stream->buf with SO_BUFF_SIZE bytes or until EOF 
 * @return Bytes read or -1 if an error occurred 
 */
int refill_buffer(SO_FILE *stream) {
    int bytes_read = so_read(stream->fd, stream->buf, SO_BUFF_SIZE);
    if (bytes_read < 0) {
        stream->error = errno;
        return -1;
    }
    if (bytes_read < SO_BUFF_SIZE) {
        stream->eof = 1;
    }
    stream->buf_cursor = 0;
    stream->buf_end = bytes_read;
    return bytes_read;
}

/**
 * @brief  Reads nmemb elements of given size into ptr from the stream
 * @return Characters read or 0 if an error occured
 */
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    int bytes_available, bytes_copied = 0, bytes_left = nmemb * size, bytes_to_copy, ret;
    if (stream->last_op == SO_WRITE_LAST) {
        // TODO: flush the buffer
    }
    stream->last_op = SO_READ_LAST;

    // Copy Min(bytes_left, bytes_available) from buffer into ptr
    bytes_available = stream->buf_end - stream->buf_cursor;
    bytes_to_copy = bytes_available < bytes_left ? bytes_available : bytes_left;
    memcpy(ptr, stream->buf, bytes_to_copy);
    bytes_left -= bytes_to_copy;
    bytes_copied += bytes_to_copy;
    // If more bytes are needed to be copied still
    if (bytes_left > 0) {
        // If bytes_left is greater than SO_BUFF_SIZE, read directly into ptr
        if (bytes_left > SO_BUFF_SIZE) {
            ret = so_read(stream->fd, ((char *)ptr) + bytes_copied, bytes_left);
            if (ret < bytes_left) {
                if (ret < 0) {
                    stream->error = 1;
                    return 0;
                }
                stream->eof = 1;
            }
            bytes_copied += ret;
            bytes_left -= ret;
        }
        else { // Refill buffer and copy the remainder of the requested bytes
            ret = refill_buffer(stream);
            if (ret < SO_BUFF_SIZE) {
                if (ret < 0) {
                    stream->error = 1;
                    return 0;
                }
                stream->eof = 1;
            }
            bytes_to_copy = ret < bytes_left ? ret : bytes_left;
            memcpy(((char *)ptr) + bytes_copied, stream->buf, bytes_to_copy);
            stream->buf_cursor += bytes_to_copy;
            bytes_copied += bytes_to_copy;
            bytes_left -= bytes_to_copy;
        }
    }
    // Check if ptr wasn't filled and set it's remainder to zero, including a
    // potential partial last element
    ret = bytes_copied % size;
    if (bytes_left > 0) {
        memset(((char *)ptr) + bytes_copied - ret, 0, nmemb - bytes_copied + ret);
    }
    return bytes_copied / size;
}

/**
 * @brief Reads one character from stream and returns it
 * @return Character read or SO_EOF in case of failure
 */
int so_fgetc(SO_FILE *stream) {
    if (stream->last_op == SO_WRITE_LAST) {
        // TODO: flush the buffer
    }
    stream->last_op = SO_READ_LAST;

    // Check if data is available in the buffer
    if (stream->buf_end - stream->buf_cursor < 1) {
        // Refill buffer
        if (refill_buffer(stream) <= 0) {
            return SO_EOF;
        }
    }
    return (int)stream->buf[stream->buf_cursor++];
}

/**
 * @brief Resets the buffer's internal cursors to show it's empty and any
 * contents are to be disregarded
 */
void reset_buffer(SO_FILE *stream) {
    stream->buf_cursor = 0;
    stream->buf_end = 0;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
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