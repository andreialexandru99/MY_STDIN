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
		return open(pathname, O_WRONLY | O_APPEND | O_CREAT, 0644);
	} else if (strcmp(mode, "a+") == 0) {
		return open(pathname, O_RDWR | O_APPEND | O_CREAT, 0644);
	} else {
		// File mode unknown
		return -1;
	}
}

/**
 * @brief Refills stream->buf with BUFF_SIZE bytes or until EOF
 * @return Bytes read or -1 if an error occurred
 */
long refill_buffer(MY_FILE *stream)
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
void reset_buffer(MY_FILE *stream)
{
	stream->buf_cursor = 0;
	stream->buf_end = 0;
}

/**
 * @brief Copies bytes from the stream buffer into ptr. The number of bytes
 * copied is either the one given through the parameter bytes if it's smaller
 * than the size of the valid buffer contents, or the size of the buffer
 * contents
 * @return Number of bytes copied
 */
size_t copy_from_buf(MY_FILE *stream, void *ptr, size_t bytes) {
    size_t bytes_available = stream->buf_end - stream->buf_cursor;
    size_t bytes_to_copy = bytes_available < bytes ? bytes_available : bytes;

    memcpy(ptr, stream->buf + stream->buf_cursor, bytes_to_copy);
    stream->buf_cursor += bytes_to_copy;
    return bytes_to_copy;
}

/**
 * @brief Copies bytes from ptr into the stream buffer. The number of bytes
 * copied is either the one given through the parameter bytes if it's smaller
 * than the available space in the buffer, or the size of the available space
 * in the buffer
 * @return Number of bytes copied
 */
size_t copy_into_buf(MY_FILE *stream, const void *ptr, size_t bytes) {
	size_t bytes_available = BUFF_SIZE - stream->buf_end;
	size_t bytes_to_copy = bytes_available < bytes ? bytes_available : bytes;
	
	memcpy(stream->buf + stream->buf_end, ptr, bytes_to_copy);
	stream->buf_end += bytes_to_copy;
	return bytes_to_copy;
}

/**
 * @brief Creates a new process which executes the given command. Depending on
 * the mode, the appropriate ends of the pipe given through fds are closed in
 * the child and parent processes. The stream's fd field is set to the parent
 * process' end of the pipe.
 * @return 0 if successful, -1 if an error occurred.
 */
int start_proc(MY_FILE *stream, int *fds, int mode, const char *command) {
	int pid, ret;

	pid = fork();
	switch (pid) {
		case -1: // Forking failed
			return -1;
		case 0: // Child process
			close(fds[mode]);
			if (mode == 0) // Parent in reading mode. Redirect stdout
				ret = dup2(fds[1], STDOUT_FILENO);
			else // Parent in writting mode. Redirect stdin
				ret = dup2(fds[0], STDIN_FILENO);
			if (ret == -1)
				exit(-2);
			// Execute given command
			ret = execlp("sh", "sh", "-c", command, NULL);
			if (ret == -1)
				exit(-1);
			break;
		default: // Parent process
			stream->child_pid = pid;
			close(fds[1 - mode]);
			stream->fd = fds[mode];
			break;
	}
	return 0;
}
