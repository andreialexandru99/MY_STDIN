CC = gcc
CFLAGS = -Wall -fPIC

build: libso_stdio.so

libso_stdio.so: so_stdio.o
	$(CC) -shared $^ -o $@

so_stdio.o: so_stdio.c
	$(CC) $(CFLAGS) $^ -c

clean:
	rm so_stdio.o libso_stdio.so

.PHONY: clean