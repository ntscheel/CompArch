CC=gcc
CFLAGS=-pipe

all: cache_sim
	$(CC) $(CFLAGS) cache_sim.o -o cache_sim -lm
cache_sim: cache_sim.c
	$(CC) $(CFLAGS) -c cache_sim.c -o cache_sim.o -lm
clean:
	rm -f *.o
	rm -f cache_sim