CC = gcc
CFLAGS = -Wall -fPIC

build: libmy_stdio.so

libmy_stdio.so: my_stdio.o util.o
	$(CC) -shared $^ -o $@

my_stdio.o: src/my_stdio.c
	$(CC) $(CFLAGS) $^ -c

util.o: src/util.c
	$(CC) $(CFLAGS) $^ -c

clean:
	rm my_stdio.o util.o libmy_stdio.so

.PHONY: clean