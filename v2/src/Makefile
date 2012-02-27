.PHONY: clean,all
CC=gcc
CFLAGS=-g -Wall 

all: tstserver test_tst
	cp tstserver ../bin/

tstserver: tstserver.o buffer_pool.o tst_db.o cmd_process.o
	$(CC) $(CFLAGS) tstserver.o buffer_pool.o tst_db.o cmd_process.o -o tstserver -lpthread

buffer_pool.o: buffer_pool.c buffer_pool.h
	$(CC) $(CFLAGS) -c buffer_pool.c -o buffer_pool.o

cmd_process.o: cmd_process.c cmd_process.h tstserver.h
	$(CC) $(CFLAGS) -c cmd_process.c -o cmd_process.o

tstserver.o: tstserver.c tstserver.h
	$(CC) $(CFLAGS)  -c tstserver.c -o tstserver.o

tst_db.o: tst.c tst.h
	$(CC) $(CFLAGS) -c -O2 tst.c -o tst_db.o

test_tst: test_tst.c tst_db.o
	$(CC) $(CFLAGS) tst_db.o test_tst.c -o test_tst

clean:
	rm *.o; rm tstserver
