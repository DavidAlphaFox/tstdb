#include "tst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static tst_db *new_tst_db(uint32 cap)
{
	tst_db *db = (tst_db *) malloc(sizeof(tst_db));
	db->size = 0;
	db->root = 0;
	db->f_head = 0;
	db->cap = cap;
	db->data = (tst_node *) malloc(sizeof(tst_node) * cap);
	memset(&db->data[0],0,sizeof(db->data));
	return db;
}

tst_db *create_tst_db()
{
	return new_tst_db(409600);
}

void free_tst_db(tst_db * db)
{
	if (db != NULL) {
		if (db->data != NULL) {
			free(db->data);
		}
		free(db);
	}
}

static void ensure_enough_space(tst_db * db)
{
	tst_node *new_p;
	if (db != NULL) {
		if (db->size >= db->cap - 1) {
			printf(">> allocate new memory\n");
			new_p =
			    (tst_node *) realloc(db->data,
						 sizeof(tst_node) *
						 (db->cap) * 2);
			if (new_p == NULL) {
				printf("no enough space");
				exit(1);
			}
			db->data = new_p;
			db->cap = (db->cap) * 2;
		}
	}
}

static void free_node(tst_db *db, uint32 node)
{
	db->data[node].mid = db->f_head;
	db->data[node].value = 0;
	db->f_head = node;	
}
	
static uint32 new_node(tst_db * db)
{
	uint32 tmp;
	if(db->f_head==0){
			ensure_enough_space(db);
			db->size++;
			memset(&db->data[db->size], 0, sizeof(tst_node));
			db->data[db->size].value = 0;
			return db->size;
	}else{
			tmp=db->f_head;
			db->f_head=db->data[db->f_head].mid;
			memset(&db->data[tmp],0, sizeof(tst_node));
			return tmp;	
	}
}

static uint32 insert(tst_db * db, uint32 node, const char *s, uint64 value,
		     int d, int len_of_s, uint32 parent)
{
	char c = s[d];
	uint32 x = node, no;
	if (x == 0) {
		x = new_node(db);
		db->data[x].c = c;
	}
	if (c < db->data[x].c) {
		no = insert(db, db->data[x].left, s, value, d, len_of_s, x);
		db->data[x].left = no;
	} else if (c > db->data[x].c) {
		no = insert(db, db->data[x].right, s, value, d, len_of_s, x);
		db->data[x].right = no;
	} else if (d < len_of_s - 1) {

		no = insert(db, db->data[x].mid, s, value, d + 1, len_of_s, x);
		db->data[x].mid = no;
	} else {

		db->data[x].value = value;
	}
	db->data[x].parent = parent;	

	return x;
}

static uint32 search(tst_db * db,  const char *key, int len_of_s)
{
	int len_of_key = strlen(key);
	uint32 node = db->root;
	char c;
	int d=0;

	if (len_of_key <= 0)
		return 0;

	while(node){
		c = key[d];
		if( c < db->data[node].c){
			node = db->data[node].left;
		}else if( c > db->data[node].c ){
			node = db->data[node].right;
		}else if( d < len_of_key-1 ) {
			node = db->data[node].mid;
			d++;		
		}else{
			break;
		}
	}		

	return node;
}

static uint32 search_with_path(tst_db * db,  const char *key, int len_of_s,char* path)
{
	int len_of_key = strlen(key);
	uint32 node = db->root;
	char c;
	int d=0;

	if (len_of_key <= 0)
		return 0;
	while(node){
		c = key[d];
		if( c < db->data[node].c){
			node = db->data[node].left;
		}else if( c > db->data[node].c ){
			node = db->data[node].right;
		}else if( d < len_of_key-1 ) {
			path[d]=db->data[node].c;
			node = db->data[node].mid;
			d++;		
		}else{
			path[d]=db->data[node].c;
			break;
		}
	}		

	return node;
}

void tst_put(tst_db * db, const char *key, uint64 value)
{
	int len_of_key = strlen(key);
	if (len_of_key <= 0)
		return;
	db->root =
	    insert(db, db->root, key, value, 0, len_of_key ,0 );
}

static uint32 remove_node(tst_db * db, uint32 node)
{

	uint32 max_p;
	uint32 left = db->data[node].left;
	uint32 right = db->data[node].right;

	//printf("remove %c\n",db->data[node].c);

	free_node(db, node);

	if(left== 0 && right == 0){
			return 0;
	}else if(left >0 && right == 0){
			return left;	
	}else if(right>0 && left == 0){
			return right;	
	}else{
			max_p = left;
			while(db->data[max_p].right){
				max_p = db->data[max_p].right;
			}							
			db->data[max_p].right = right;
			return left;			
	}
}

void tst_delete(tst_db *db,const char *key)
{
	int len_of_key = strlen(key);
	uint32 node = db->root;
	char c;
	int d=0;
	uint32 myparent=0;
	uint32 new_p;

	if (len_of_key <= 0)
		return;

	while(node){
		c = key[d];
		if( c < db->data[node].c){
			node = db->data[node].left;
		}else if( c > db->data[node].c ){
			node = db->data[node].right;
		}else if( d < len_of_key-1 ) {
			node = db->data[node].mid;
			d++;		
		}else{
			break;
		}
	}	

	if(node){
		db->data[node].value = 0;
	}	
	while(node){ //found it
		myparent = db->data[node].parent;

		if(db->data[node].mid == 0 && db->data[node].value==0){ //can delete
			if(myparent >0){
				if(node == db->data[myparent].left){ //is left child
					new_p = remove_node(db, node);
					db->data[myparent].left = new_p;	
					if(new_p){
						db->data[new_p].parent = myparent;
					}
				}
				else if(node == db->data[myparent].right){ //is right child
					new_p = remove_node(db, node);
					db->data[myparent].right = new_p;
					if(new_p){
						db->data[new_p].parent = myparent;
					}
				}
				else{ //is middle child
					new_p = remove_node(db, node);
					db->data[myparent].mid = new_p; 
					if(new_p){
						db->data[new_p].parent = myparent;
					}
				}
			}else{
				new_p = remove_node(db,node) ;
				db->root = new_p;
				db->data[new_p].parent = myparent;
			}
		}		
		node = myparent;		
	}	
}

 
static
void append_result( const char* key, char* result, int * result_size)
{
	//printf("append: %s\n",key);
	strcat(result, key);	
	strcat(result, "\r\n");
	*result_size = (*result_size) +1;
}


static
void dfs(tst_db *db, uint32 node, char* result, int* result_size,
         char key_buf[],int d,int limit)
{
	if(node==0)
		return;

	if(*result_size >= limit)
		return;
	
	dfs(db, db->data[node].left, result, result_size,key_buf,d,limit);
	key_buf[d]=db->data[node].c;
	dfs(db, db->data[node].mid, result, result_size,key_buf,d+1,limit);
	key_buf[d]=db->data[node].c;	
	dfs(db, db->data[node].right, result,result_size,key_buf, d, limit);	
	key_buf[d]=db->data[node].c;	

	if(db->data[node].value){
		key_buf[d+1]='\0';
		append_result(key_buf, result, result_size);
	}	
}


void tst_prefix(tst_db *db, const char* prefix,char* result,
               int* result_size,int limit)
{
	int len_of_prefix = strlen(prefix);
	char base_key[MAX_KEY_SIZE]={0};
	if (len_of_prefix <= 0)
		return ;
	uint32 node = search_with_path(db, prefix, len_of_prefix,base_key);
	if(node==0)
		return ;
	result[0]='\0';
	*result_size=0;
	if(db->data[node].value && *result_size < limit)	
		append_result(base_key, result, result_size);

	if(*result_size < limit)
		dfs(db, db->data[node].mid, result, result_size,base_key, strlen(base_key), limit);		
}


uint64 tst_get(tst_db * db, const char *key)
{
	int len_of_key = strlen(key);
	if (len_of_key <= 0)
		return 0;
	uint32 node = search(db, key, len_of_key);
	return db->data[node].value;
}
