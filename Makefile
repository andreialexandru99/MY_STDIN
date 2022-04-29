CC = gcc
CFLAGS = -Wall

build: libmy_stdio.a

libmy_stdio.a: my_stdio.o util.o
	$(CC) -shared $^ -o $@
	ar -rc $@ $^

my_stdio.o: src/my_stdio.c src/my_stdio.h
	$(CC) $(CFLAGS) $< -c

util.o: src/util.c src/util.h
	$(CC) $(CFLAGS) $< -c

clean:
	rm my_stdio.o util.o libmy_stdio.a

.PHONY: clean