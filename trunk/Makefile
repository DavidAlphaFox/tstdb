CC=gcc

CFLAGS=-c -Wall -O3
CCLINK=-lpthread

all: bench server test lib

server: tstserver.o tst.o
	$(CC) $(CCLINK) tstserver.o tst.o -o tstserver

lib: tst.o
	$(CC) -shared tst.o -o libtst.so

bench: tst.o bench.o
	$(CC) tst.o bench.o -o bench
test: tst.o test.o
	$(CC) tst.o test.o -o tsttest

tstserver.o: tstserver.c
	$(CC) -c -Wall tstserver.c -o tstserver.o

tst.o: tst.c
	$(CC) $(CFLAGS) -fpic tst.c

test.o: test.c
	$(CC) $(CFLAGS) test.c

bench.o: bench.c
	$(CC) $(CFLAGS) bench.c
clean:
	rm -rf *o tsttest bench tstserver


