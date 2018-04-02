CC=gcc
CFLAGS=-pipe

all: assembler
	$(CC) $(CFLAGS) assembler.o -o assembler
assembler: assembler.c
	$(CC) $(CFLAGS) -c assembler.c -o assembler.o
clean:
	rm -f *.o
	rm -f assembler