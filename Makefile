#compiler variables
CC=gcc -std=c99
CFLAGS=-c -g

smallsh: smallsh.o
	$(CC) *.o -o smallsh 

smallsh.o: smallsh.c
	$(CC) $(CFLAGS) smallsh.c

clean:
	rm smallsh smallsh.o
