CC=gcc
CFLAGS=-pipe

all: simulator
	$(CC) $(CFLAGS) simulator.o -o simulator
simulator: simulator.c
	$(CC) $(CFLAGS) -c simulator.c -o simulator.o
clean:
	rm -f *.o
	rm -f simulator