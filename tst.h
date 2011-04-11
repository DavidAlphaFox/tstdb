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
	tst_node* data;//data[0] is not used
}tst_db;

//public functions
tst_db* tst_open(const char* full_file_name);
void tst_close(tst_db* db);
void tst_put(tst_db* db,const char* key, const char* value, uint32 len_of_value);
void tst_get(tst_db* db,const char* key, char* value,uint32* len_of_value_ptr);


//private functions
tst_db* new_tst_db(uint32 cap);
tst_db* create_tst_db();
void free_tst_db(tst_db* db);
void ensure_enough_space(tst_db *db);
void put(tst_db *db,const char* str,uint64 value,char small_value_len);
long get(tst_db *db,const char* str);
uint32 get_node(tst_db *db, const char* str);

uint32 insert(tst_db *db,uint32 node,const char* s,uint64 value,int d,int len_of_s,char small_value_len);
uint32 search(tst_db *db,uint32 node,const char* s,int d,int len_of_s);
uint32 new_node(tst_db *db);
#endif
