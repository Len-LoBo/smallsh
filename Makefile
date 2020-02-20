#compiler variables
CC=gcc -std=gnu99
CFLAGS=-c -g

smallsh: smallsh.o dynArray.o
	$(CC) *.o -o smallsh -lm 

smallsh.o: smallsh.c
	$(CC) $(CFLAGS) smallsh.c

clean:
	rm smallsh smallsh.o
