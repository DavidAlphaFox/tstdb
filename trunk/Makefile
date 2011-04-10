CC=gcc

CFLAGS=-c -Wall -O3

all: bench lib test

lib: tst.o
	$(CC) -shared -fPIC tst.o -o libtst.so

bench: tst.o bench.o
	$(CC) tst.o bench.o -o bench
test: tst.o test.o
	$(CC) tst.o test.o -o tsttest

tst.o: tst.c
	$(CC) $(CFLAGS) tst.c

test.o: test.c
	$(CC) $(CFLAGS) test.c

bench.o: bench.c
	$(CC) $(CFLAGS) bench.c
clean:
	rm -rf *o tsttest bench


