#ifndef MY_STDIO_H
#define MY_STDIO_H

#include <stdlib.h>

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

#define MY_EOF (-1)

struct _my_file;
typedef struct _my_file MY_FILE;

MY_FILE *my_fopen(const char *pathname, const char *mode);

int my_fclose(MY_FILE *stream);

int my_fileno(MY_FILE *stream);

int my_fflush(MY_FILE *stream);

int my_fseek(MY_FILE *stream, long offset, int whence);

long my_ftell(MY_FILE *stream);

size_t my_fread(void *ptr, size_t size, size_t nmemb, MY_FILE *stream);

size_t my_fwrite(const void *ptr, size_t size, size_t nmemb, MY_FILE *stream);

int my_fgetc(MY_FILE *stream);

int my_fputc(int c, MY_FILE *stream);

int my_feof(MY_FILE *stream);

int my_ferror(MY_FILE *stream);

MY_FILE *my_popen(const char *command, const char *type);

int my_pclose(MY_FILE *stream);

#endif /* MY_STDIO_H */
