#ifndef TSTDB
#define TSTDB "tstdb"
#include <stdio.h>
#define MAX_KEY_SIZE 250

typedef unsigned int uint32;
typedef unsigned long long uint64;

#pragma pack(1)
typedef struct{
	char c;
	uint32 left,mid,right,parent;
	uint64 value;	
}tst_node;
#pragma pack()


typedef struct{
	uint32 root;
	uint32 size;
	uint32 cap;
	uint32 f_head; //head of reuseable node list
	tst_node* data;//data[0] is not used
}tst_db;

//public functions
tst_db *create_tst_db();
void tst_put(tst_db * db, const char *key, uint64 value);
uint64 tst_get(tst_db * db, const char *key);
void tst_prefix(tst_db *db, const char* prefix,char result[][MAX_KEY_SIZE],int* result_size);
void tst_delete(tst_db *db, const char * key);
void free_tst_db(tst_db * db);

#endif
