CC=gcc
CFLAGS=-pipe

all: pipeline-sim
	$(CC) $(CFLAGS) pipeline-sim.o -o pipeline-sim
pipeline-sim: pipeline-sim.c
	$(CC) $(CFLAGS) -c pipeline-sim.c -o pipeline-sim.o
clean:
	rm -f *.o
	rm -f pipeline-sim