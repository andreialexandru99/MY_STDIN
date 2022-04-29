#include "my_stdio.h"
#include "util.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Opens file from pathname in given mode and initializes the MY_FILE structure
 * @return Pointer to MY_FILE structure associated or NULL if an error occurred
 */
MY_FILE *my_fopen(const char *pathname, const char *mode)
{
	// Allocate memory for MY_FILE structure
	MY_FILE *file = malloc(sizeof(MY_FILE));

	if (file == NULL)
		return NULL;
	// Allocate memory for IO buffer
	file->buf = calloc(BUFF_SIZE, sizeof(char));
	if (file->buf == NULL) {
		free(file);
		return NULL;
	}
	// Initialize file indicators
	file->error = 0;
	file->eof = 0;
	file->buf_cursor = 0;
	file->buf_end = 0;
	file->last_op = NO_OP;
	// Open file
	file->fd = open_file(pathname, mode);
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
 * @return 0 if successful or MY_EOF if an error occurred
 */
int my_fclose(MY_FILE *stream)
{
	int ret;

	// Check if flushing is needed
	if (stream->last_op == WRITE_LAST) {
		ret = my_fflush(stream);
		if (ret == MY_EOF) {
			close(stream->fd);
			free(stream->buf);
			free(stream);
			return MY_EOF;
		}
	}
	// Close fd and free memory
	ret = close(stream->fd);
	free(stream->buf);
	free(stream);
	if (ret < 0)
		return MY_EOF;
	return 0;
}

/**
 * @return The file descriptor of the stream
 */
int my_fileno(MY_FILE *stream)
{
	return stream->fd;
}

/**
 * @brief Flushes the contents of the stream's internal buffer
 * @return 0 if successful, MY_EOF if an error occurred
 */
int my_fflush(MY_FILE *stream)
{
	long ret, bytes;

	// If last operation was a reading one, flushing fails
	if (stream->last_op == READ_LAST)
		return MY_EOF;
	// Write the buffer contents into the stream
	bytes = stream->buf_end - stream->buf_cursor;
	if (bytes > 0) {
		ret = write(stream->fd, stream->buf, bytes);
		if (ret <= 0) {
			stream->error = -1;
			return MY_EOF;
		}
	}
	stream->buf_cursor = 0;
	stream->buf_end = 0;
	return 0;
}

/**
 * @brief Changes the cursor's position in the stream to offset bytes from the position
 * given through whence. Clears eof indicator
 * @return 0 if successful, -1 if an error occured
 */
int my_fseek(MY_FILE *stream, long offset, int whence)
{
	long ret;

	// Flush/reset the buffer if needed
	if (stream->last_op == WRITE_LAST) {
		ret = my_fflush(stream);
		if (ret == MY_EOF)
			return -1;
	} else {
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

/**
 * @brief Calculates the position in the stream while accounting for the internal buffer
 * @return Position in stream or -1 if an error occured
 */
long my_ftell(MY_FILE *stream)
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
	if (stream->last_op == READ_LAST) {
		// Ignore bytes read in buffer that haven't yet been read by user
		pos -= bytes_in_buffer;
	} else {
		// Add bytes written in buffer that haven't yet been flushed
		pos += bytes_in_buffer;
	}
	return pos;
}

/**
 * @brief  Reads nmemb elements of given size into ptr from the stream
 * @return Elements read or 0 if an error occured
 */
size_t my_fread(void *ptr, size_t size, size_t nmemb, MY_FILE *stream)
{
	size_t bytes_read = 0, bytes_left = nmemb * size;
	long ret;

	// Update stream's last operation
	stream->last_op = READ_LAST;
	// Copy buffer contents into ptr and update bytes_left.
	ret = copy_from_buf(stream, ptr, bytes_left);
	bytes_left -= ret;
	bytes_read += ret;
	// Check if there are bytes left needed to be read
	if (bytes_left > BUFF_SIZE) {
		// Number of bytes left needed is greater than the size of the buffer
		// Read directly into ptr to avoid unnecessary system calls
		ret = read(stream->fd, ((char *)ptr) + bytes_read, bytes_left);
		bytes_read += ret;
		if (ret < 0) {
			stream->error = -1;
			return 0;
		}
		if (ret == 0) {
			stream->eof = 1;
			return bytes_read / size;
		}
	} else if (bytes_left > 0) {
		// Number of bytes left needed fits inside the buffer. Refill for potential
		// future reads and copy bytes_left bytes into ptr.
		ret = refill_buffer(stream);
		if (ret < 0)
			return 0;
		if (ret == 0) {
			stream->eof = 1;
			return bytes_read / size;
		}
		bytes_read += copy_from_buf(stream, ((char *)ptr) + bytes_read, bytes_left);
	}
	return bytes_read / size;
}

/**
 * @brief Reads one character from stream and returns it
 * @return Character read or MY_EOF in case of failure
 */
int my_fgetc(MY_FILE *stream)
{
	long ret;

	// Update stream's last operation
	stream->last_op = READ_LAST;
	// Check if data is available in the buffer
	if (stream->buf_end - stream->buf_cursor < 1) {
		// Refill buffer
		ret = refill_buffer(stream);
		if (ret == 0)
			stream->eof = 1;
		if (ret <= 0)
			return MY_EOF;
	}
	return (int)stream->buf[stream->buf_cursor++];
}

/**
 * @brief Writes nmemb elements of given size from ptr into the stream
 * @return Number of written bytes or 0 if an error occured
 */
size_t my_fwrite(const void *ptr, size_t size, size_t nmemb, MY_FILE *stream)
{
	size_t bytes_left = size * nmemb, bytes_written = 0;
	long ret;

	// Update stream's last operation and reset eof
	stream->last_op = WRITE_LAST;
	stream->eof = 0;
	// Copy the bytes that fit into the buffer
	ret = copy_into_buf(stream, ptr, bytes_left);
	bytes_left -= ret;
	bytes_written += ret;
	// Check if more bytes need to be written
	if (bytes_left > BUFF_SIZE) {
		// Number of bytes left to be written exceed BUFF_SIZE. Flush and write
		// the rest directly into the file
		if (my_fflush(stream) == MY_EOF)
			return 0;
		ret = write(stream->fd, ((char *)ptr) + bytes_written, bytes_left);
		if (ret <= 0) {
			stream->error = -1;
			return 0;
		}
		bytes_written += ret;
	} else if (bytes_left > 0) {
		// Number of bytes left to be written fit into the buffer. Flush and
		// copy what's left from ptr
		if (my_fflush(stream) == MY_EOF)
			return 0;
		bytes_written += copy_into_buf(stream, ((char *)ptr) + bytes_written, bytes_left);
	}
	return bytes_written / size;
}

/**
 * @brief Writes given character into the stream
 * @return Written character or MY_EOF if an error occured
 */
int my_fputc(int c, MY_FILE *stream)
{
	long ret;

	// Update stream's last operation and reset eof
	stream->last_op = WRITE_LAST;
	stream->eof = 0;
	// Check if buffer needs flushing for space
	if (BUFF_SIZE - stream->buf_end < 1) {
		// Flush the stream
		ret = my_fflush(stream);
		if (ret == MY_EOF)
			return MY_EOF;
	}
	// Copy character into buffer and return it
	stream->buf[stream->buf_end] = (char)c;
	return (int)stream->buf[stream->buf_end++];
}

/**
 * @return Returns the eof indicator of the stream: 1 if reached, 0 otherwise
 */
int my_feof(MY_FILE *stream)
{
	return stream->eof;
}

/**
 * @return Returns the error indicator of the stream or 0 if none occurred
 */
int my_ferror(MY_FILE *stream)
{
	return stream->error;
}

/**
 * @brief Starts a new process that will execute the given command and opens a pipe
 * to read from/write to based on the given type
 * @return MY_FILE structure used to communicate with the process or NULL if an
 * error occurred
 */
MY_FILE *my_popen(const char *command, const char *type)
{
	// Allocate memory for MY_FILE structure
	MY_FILE *file = malloc(sizeof(MY_FILE));
	int fds[2], ret, mode;

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
	file->buf = calloc(BUFF_SIZE, sizeof(char));
	if (file->buf == NULL) {
		free(file);
		return NULL;
	}
	// Initialize file indicators
	file->error = 0;
	file->eof = 0;
	file->buf_cursor = 0;
	file->buf_end = 0;
	file->last_op = NO_OP;
	// Create pipe
	ret = pipe(fds);
	if (ret == -1) {
		free(file->buf);
		free(file);
		return NULL;
	}
	ret = start_proc(file, fds, mode, command);
	if (ret == -1) {
		close(fds[0]);
		close(fds[1]);
		free(file->buf);
		free(file);
		return NULL;
	}
	return file;
}

/**
 * @brief Waits for the process to end and frees all memory associated with it
 * @return The status of the process or -1 if closing fails
 */
int my_pclose(MY_FILE *stream)
{
	int status, ret;

	// Check if flushing is needed
	if (stream->last_op == WRITE_LAST) {
		ret = my_fflush(stream);
		if (ret == MY_EOF) {
			close(stream->fd);
			free(stream->buf);
			free(stream);
			return -1;
		}
	}
	close(stream->fd);
	// Wait for child process to finish
	ret = waitpid(stream->child_pid, &status, 0);
	if (ret == -1)
		status = -1;
	// Close the stream and free all memory associated with it
	free(stream->buf);
	free(stream);
	return status;
}
