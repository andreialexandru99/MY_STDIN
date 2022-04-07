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
		file->fd = open(pathname, O_APPEND | O_CREAT, 0644);
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
 * @brief Closes given stream and frees all memory associated with the it
 * @return 0 if successful or SO_EOF if an error occurred
 */
int so_fclose(SO_FILE *stream)
{
	int ret;

	// Check if flushing is needed
	if (stream->last_op == SO_WRITE_LAST) {
		ret = so_fflush(stream);
		if (ret == SO_EOF)
			return 0;
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
	long ret;

	// If last operation was a reading one, flushing fails
	if (stream->last_op == SO_READ_LAST)
		return SO_EOF;
	// Write the buffer contents into the stream
	ret = so_write(stream->fd, stream->buf, stream->buf_end - stream->buf_cursor);
	if (ret == 0) {
		stream->error = errno;
		return SO_EOF;
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
		stream->error = errno;
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
	if (pos < 0)
		stream->error = errno;
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
 * @brief Reads from fd into buf until nbytes or EOF reached
 * @return Bytes read or a negative value if an error occurred
 */
long so_read(int fd, void *buf, size_t nbytes)
{
	long bytes_read = 0, ret;

	// Read in a loop until nbytes have been read or eof is encountered
	while (bytes_read < nbytes) {
		ret = read(fd, ((char *)buf) + bytes_read, nbytes - bytes_read);
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
		bytes_read += ret;
		break;
	}
	return bytes_read;
}

/**
 * @brief Refills stream->buf with SO_BUFF_SIZE bytes or until EOF
 * @return Bytes read or -1 if an error occurred
 */
long refill_buffer(SO_FILE *stream)
{
	long bytes_read = so_read(stream->fd, stream->buf, SO_BUFF_SIZE);

	if (bytes_read < 0) {
		stream->error = errno;
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

	// Set last operation as SO_READ_LAST for so_fseek
	stream->last_op = SO_READ_LAST;
	// Copy Min(bytes_left, bytes_available) from buffer into ptr
	bytes_available = stream->buf_end - stream->buf_cursor;
	bytes_to_copy = bytes_available < bytes_left ? bytes_available : bytes_left;
	memcpy(ptr, stream->buf, bytes_to_copy);
	bytes_left -= bytes_to_copy;
	bytes_copied += bytes_to_copy;
	stream->buf_cursor += bytes_to_copy;
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
		} else { // Refill buffer and copy the remainder of the requested bytes
			ret = refill_buffer(stream);
			if (ret < 0)
				return 0;
			if (ret == 0)
				stream->eof = 1;
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
	if (bytes_left > 0)
		memset(((char *)ptr) + bytes_copied - ret, 0, nmemb - bytes_copied + ret);
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
 * @brief Writes nbytes bytes from buf into fd, or until there is no more room
 * for data
 * @return Number of bytes written or 0 if an error occurred
 */
long so_write(int fd, const void *buf, size_t nbytes)
{
	long bytes_written = 0, ret;

	while (bytes_written < nbytes) {
		ret = write(fd, ((char *)buf) + bytes_written, nbytes - bytes_written);
		if (ret <= 0)
			return 0;
		bytes_written += ret;
	}
	return bytes_written;
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
	long bytes_left = size * nmemb, bytes_available, bytes_copied = 0, ret;

	// Set last operation as SO_WRITE_LAST for so_fseek and reset eof
	stream->last_op = SO_WRITE_LAST;
	stream->eof = 0;
	// Check if writting should be made into the buffer or stream
	bytes_available = SO_BUFF_SIZE - stream->buf_end;
	if (bytes_left > bytes_available) {
		// Flush the stream
		ret = so_fflush(stream);
		if (ret == SO_EOF)
			return 0;
		// Write directly into the stream
		ret = so_write(stream->fd, ptr, bytes_left);
		if (ret == 0) {
			stream->error = errno;
			return 0;
		}
		bytes_copied += ret;
	} else {
		// copy bytes_left into buffer
		memcpy(stream->buf + stream->buf_end, ptr, bytes_left);
		bytes_copied += bytes_left;
		stream->buf_end += bytes_left;
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

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return -1;
}
