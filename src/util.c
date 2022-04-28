#include "util.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Opens file given by pathname in the requested mode
 * @return file descriptor of the opened file or -1 if opening failed
 */
int open_file(const char *pathname, const char *mode) {
	// Open file
	if (strcmp(mode, "r") == 0) {
		return open(pathname, O_RDONLY);
	} else if (strcmp(mode, "r+") == 0) {
		return open(pathname, O_RDWR);
	} else if (strcmp(mode, "w") == 0) {
		return open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	} else if (strcmp(mode, "w+") == 0) {
		return open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0644);
	} else if (strcmp(mode, "a") == 0) {
		return open(pathname, O_WRONLY || O_APPEND | O_CREAT, 0644);
	} else if (strcmp(mode, "a+") == 0) {
		return open(pathname, O_RDWR | O_APPEND | O_CREAT, 0644);
	} else {
		// File mode unknown
		return -1;
	}
}

/**
 * @brief Refills stream->buf with SO_BUFF_SIZE bytes or until EOF
 * @return Bytes read or -1 if an error occurred
 */
long refill_buffer(SO_FILE *stream)
{
	long bytes_read = read(stream->fd, stream->buf, BUFF_SIZE);

	if (bytes_read < 0) {
		stream->error = -1;
		return -1;
	}
	// Update buffer limits
	stream->buf_cursor = 0;
	stream->buf_end = bytes_read;
	return bytes_read;
}

/**
 * @brief Resets the buffer's internal cursors to show it's empty and any
 * contents are to be disregarded
 */
void reset_buffer(SO_FILE *stream)
{
	stream->buf_cursor = 0;
	stream->buf_end = 0;
}

/**
 * @brief Copies bytes from the stream buffer into ptr. The number of bytes copied is
 * either the one given through the parameter bytes if it's smaller than the size of
 * the valid buffer contents, or the size of the buffer contents
 * @return Number of bytes copied
 */
size_t copy_from_buf(SO_FILE *stream, void *ptr, size_t bytes) {
    size_t bytes_available = stream->buf_end = stream->buf_cursor;
    size_t bytes_to_copy = bytes_available < bytes ? bytes_available : bytes;
    memcpy(ptr, stream->buf + stream->buf_cursor, bytes_to_copy);
    stream->buf_cursor += bytes_to_copy;
    return bytes_to_copy;
}
