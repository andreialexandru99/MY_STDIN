#include "so_io.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Opens file from pathname in given mode
 * @return Pointer to SO_FILE structure associated or NULL if an error occurred
 */
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	// Allocate memory for SO_FILE structure
	SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));

	if (file == NULL)
		return NULL;
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
	} else if (strcmp(mode, "r+") == 0) {
		file->fd = open(pathname, O_RDWR);
	} else if (strcmp(mode, "w") == 0) {
		file->fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	} else if (strcmp(mode, "w+") == 0) {
		file->fd = open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0644);
	} else if (strcmp(mode, "a") == 0) {
		file->fd = open(pathname, O_WRONLY || O_APPEND | O_CREAT, 0644);
	} else if (strcmp(mode, "a+") == 0) {
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
 * @brief Closes given stream and frees all memory associated with it
 * @return 0 if successful or SO_EOF if an error occurred
 */
int so_fclose(SO_FILE *stream)
{
	int ret;

	// Check if flushing is needed
	if (stream->last_op == SO_WRITE_LAST) {
		ret = so_fflush(stream);
		if (ret == SO_EOF) {
			close(stream->fd);
			free(stream->buf);
			free(stream);
			return SO_EOF;
		}
	}
	// Close fd and free memory
	ret = close(stream->fd);
	free(stream->buf);
	free(stream);
	if (ret < 0)
		return SO_EOF;
	return 0;
}

/**
 * @return The file descriptor of the stream
 */
int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

/**
 * @brief Flushes the contents of the stream's internal buffer
 * @return 0 if successful, SO_EOF if an error occurred
 */
int so_fflush(SO_FILE *stream)
{
	long ret, bytes;

	// If last operation was a reading one, flushing fails
	if (stream->last_op == SO_READ_LAST)
		return SO_EOF;
	// Write the buffer contents into the stream
	bytes = stream->buf_end - stream->buf_cursor;
	if (bytes > 0) {
		ret = write(stream->fd, stream->buf, bytes);
		if (ret <= 0) {
			stream->error = -1;
			return SO_EOF;
		}
	}
	stream->buf_cursor = 0;
	stream->buf_end = 0;
	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	long ret;

	// Check if buffer flushing/resetting is needed
	if (stream->last_op == SO_WRITE_LAST) {
		// Flush the buffer
		ret = so_fflush(stream);
		if (ret == SO_EOF)
			return 0;
	} else {
		// Reset the buffer
		reset_buffer(stream);
	}
	// Change cursor postition
	ret = lseek(stream->fd, offset, whence);
	if (ret < 0) {
		stream->error = -1;
		return -1;
	}
	// Clear eof indicator
	stream->eof = 0;
	return 0;
}

long so_ftell(SO_FILE *stream)
{
	long pos, bytes_in_buffer;

	// Get actual position in file
	pos = lseek(stream->fd, 0, SEEK_CUR);
	if (pos < 0) {
		stream->error = -1;
		return -1;
	}
	// Update position based on buffer contents
	bytes_in_buffer = stream->buf_end - stream->buf_cursor;
	if (stream->last_op == SO_READ_LAST) {
		// Ignore bytes read in buffer that haven't yet been used
		pos -= bytes_in_buffer;
	} else {
		// Add bytes written in buffer that haven't yet been flushed
		pos += bytes_in_buffer;
	}
	return pos;
}

/**
 * @brief Refills stream->buf with SO_BUFF_SIZE bytes or until EOF
 * @return Bytes read or -1 if an error occurred
 */
long refill_buffer(SO_FILE *stream)
{
	long bytes_read = read(stream->fd, stream->buf, SO_BUFF_SIZE);

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
 * @brief  Reads nmemb elements of given size into ptr from the stream
 * @return Characters read or 0 if an error occured
 */
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	long bytes_available, bytes_copied = 0, bytes_left = nmemb * size, bytes_to_copy, ret;

	// Set last operation as SO_READ_LAST for fseek
	stream->last_op = SO_READ_LAST;
	while (bytes_copied < nmemb * size) {
		bytes_available = stream->buf_end - stream->buf_cursor;
		// Ensure only whole elements are read
		bytes_available -= bytes_available % size;
		// Copy from buffer
		bytes_to_copy = bytes_available < bytes_left ? bytes_available : bytes_left;
		memcpy(((char *)ptr) + bytes_copied, stream->buf + stream->buf_cursor, bytes_to_copy);
		bytes_left -= bytes_to_copy;
		bytes_copied += bytes_to_copy;
		stream->buf_cursor += bytes_to_copy;
		// Check if buffer needs refill for more data
		if (bytes_left > 0) {
			ret = refill_buffer(stream);
			if (ret < 0)
				return 0;
			if (ret == 0) {
				stream->eof = 1;
				break;
			}
		}
	}
	return bytes_copied / size;
}

/**
 * @brief Reads one character from stream and returns it
 * @return Character read or SO_EOF in case of failure
 */
int so_fgetc(SO_FILE *stream)
{
	long ret;

	// Set last operation as SO_READ_LAST for so_fseek
	stream->last_op = SO_READ_LAST;
	// Check if data is available in the buffer
	if (stream->buf_end - stream->buf_cursor < 1) {
		// Refill buffer
		ret = refill_buffer(stream);
		if (ret == 0)
			stream->eof = 1;
		if (ret <= 0)
			return SO_EOF;
	}
	return (int)stream->buf[stream->buf_cursor++];
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
 * @brief Writes nmemb elements of given size from ptr into the stream
 * @return Number of written bytes or 0 if an error occured
 */
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	long bytes_left = size * nmemb, bytes_available, bytes_copied = 0, bytes_to_copy, ret;

	// Set last operation as SO_WRITE_LAST for so_fseek and reset eof
	stream->last_op = SO_WRITE_LAST;
	stream->eof = 0;
	while (bytes_copied < nmemb * size) {
		bytes_available = SO_BUFF_SIZE - stream->buf_end;
		// Ensure only whole elements are read
		bytes_available -= bytes_available % size;
		// Copy into buffer
		bytes_to_copy = bytes_available < bytes_left ? bytes_available : bytes_left;
		memcpy(stream->buf + stream->buf_end, ((char *)ptr + bytes_copied), bytes_to_copy);
		bytes_copied += bytes_to_copy;
		stream->buf_end += bytes_to_copy;
		bytes_left -= bytes_to_copy;
		// If more needs to be written, flush the buffer to make room
		if (bytes_left > 0) {
			ret = so_fflush(stream);
			if (ret == SO_EOF)
				return 0;
		}
	}
	return bytes_copied / size;
}

/**
 * @brief Writes c into the stream
 * @return Written character or SO_EOF if an error occured
 */
int so_fputc(int c, SO_FILE *stream)
{
	long ret;

	// Set last operation as SO_WRITE_LAST for so_fseek and reset eof
	stream->last_op = SO_WRITE_LAST;
	stream->eof = 0;
	// Check if buffer needs flushing for space
	if (SO_BUFF_SIZE - stream->buf_end < 1) {
		// Flush the stream
		ret = so_fflush(stream);
		if (ret == SO_EOF)
			return SO_EOF;
	}
	// Copy character into buffer and return it
	stream->buf[stream->buf_end] = (char)c;
	return (int)stream->buf[stream->buf_end++];
}

/**
 * @return Returns the eof indicator of the stream: 1 if reached, 0 otherwise
 */
int so_feof(SO_FILE *stream)
{
	return stream->eof;
}

/**
 * @return Returns the error indicator of the stream or 0 if none occurred
 */
int so_ferror(SO_FILE *stream)
{
	return stream->error;
}

/**
 * @brief Starts a new process that will execute the given command and opens a pipe
 * to read from/write to based on the given type
 * @return SO_FILE structure used to communicate with the process
 */
SO_FILE *so_popen(const char *command, const char *type)
{
	// Allocate memory for SO_FILE structure
	SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
	int fds[2], ret, pid, mode;

	if (file == NULL)
		return NULL;
	// Check if a valid type was given
	if (strcmp(type, "r") == 0) {
		mode = 0;
	} else if (strcmp(type, "w") == 0) {
		mode = 1;
	} else {
		free(file);
		return NULL;
	}
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
	// Create pipe
	ret = pipe(fds);
	if (ret == -1) {
		free(file->buf);
		free(file);
		return NULL;
	}
	pid = fork();
	switch (pid) {
		case -1: // Forking failed
			close(fds[0]);
			close(fds[1]);
			free(file->buf);
			free(file);
			return NULL;
		case 0: // Child process
			// Check which end of the pipe needs to be closed
			if (mode == 0) { // Reading mode
				close(fds[0]);
				// Redirect fds[1] to stdout
				ret = dup2(fds[1], STDOUT_FILENO);
				if (ret == -1)
					exit(-2);
			} else { // Writting mode
				close(fds[1]);
				// Redirect fds[0] to stdin
				ret = dup2(fds[0], STDIN_FILENO);
				if (ret == -1)
					exit(-2);
			}
			// Execute given command
			ret = execlp("sh", "sh", "-c", command, NULL);
			if (ret == -1)
				exit(-1);
			break;
		default: // Parent process
			file->child_pid = pid;
			if (mode == 0) {
				close(fds[1]);
				file->fd = fds[0];
			} else {
				close(fds[0]);
				file->fd = fds[1];
			}
			break;
	}
	return file;
}

/**
 * @brief Waits for the process to end and frees all memory associated with it
 * @return The status of the process or -1 if closing fails
 */
int so_pclose(SO_FILE *stream)
{
	int status, ret;

	// Check if flushing is needed
	if (stream->last_op == SO_WRITE_LAST) {
		ret = so_fflush(stream);
		if (ret == SO_EOF) {
			close(stream->fd);
			free(stream->buf);
			free(stream);
			return -1;
		}
	}
	// Wait for child process to finish
	ret = waitpid(stream->child_pid, &status, WNOHANG);
	if (ret == -1)
		status = -1;
	// Close the stream and free all memory associated with it
	close(stream->fd);
	free(stream->buf);
	free(stream);
	return status;
}
