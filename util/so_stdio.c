#include "so_stdio.h"

struct _so_file {
    int fd;
    char buf[4096];
    long cursor;
};