#ifndef TSTDB
#define TSTDB "tstdb"
#include <stdio.h>
typedef unsigned int uint32;
typedef unsigned long long uint64;


typedef struct{
	char c;
	char small_value_len;
	uint32 left,mid,right;
	uint64 value;	
}tst_node;


typedef struct{
	uint32 root;
	uint32 size;
	uint32 cap;
	int ready;
	FILE* write_file_ptr;
	FILE* read_file_ptr;
	FILE* hint_file_ptr;
	tst_node* data;//data[0] is not used
}tst_db;

//public functions
tst_db* tst_open(const char* full_file_name);
void tst_close(tst_db* db);
void tst_put(tst_db* db,const char* key, const char* value, uint32 len_of_value);
void tst_get(tst_db* db,const char* key, char* value,uint32* len_of_value_ptr);


#endif
