CC = gcc
CFLAGS = -Wall -fPIC

build: libso_stdio.so

libso_stdio.so: so_stdio.o
	$(CC) -shared $^ -o $@
	mv $@ ../checker-lin

so_stdio.o: so_stdio.c
	$(CC) $(CFLAGS) $^ -c

clean:
	rm *.o ../checker-lin/libso_stdio.so

.PHONY: clean