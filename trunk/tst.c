#include "tst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAGIC_NUMBER 1984

static tst_db *new_tst_db(uint32 cap)
{
	tst_db *db = (tst_db *) malloc(sizeof(tst_db));
	db->size = 0;
	db->root = 0;
	db->cap = cap;
	db->ready = 0;
	db->data = (tst_node *) malloc(sizeof(tst_node) * cap);
	return db;
}

static tst_db *create_tst_db()
{
	return new_tst_db(409600);
}

static void free_tst_db(tst_db * db)
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
		//printf("%d,%d\n",db->size,db->cap);
		if (db->size >= db->cap - 1) {
			printf(">> allocate new memory\n");
			//printf("before %c\n",db->data[1].c);
			new_p =
			    (tst_node *) realloc(db->data,
						 sizeof(tst_node) *
						 (db->cap) * 2);
			//printf("after %c\n",new_p[1].c);
			if (new_p == NULL) {
				printf("no enough space");
				exit(1);
			}
			db->data = new_p;
			db->cap = (db->cap) * 2;
		}
	}
}

static uint32 new_node(tst_db * db)
{
	ensure_enough_space(db);
	db->size++;
	memset(&db->data[db->size], 0, sizeof(tst_node));
	db->data[db->size].value = -1;
	return db->size;
}

static uint32 insert(tst_db * db, uint32 node, const char *s, uint64 value,
		     int d, int len_of_s, char small_value_len)
{
	char c = s[d];
	uint32 x = node, no;
	if (x == 0) {
		x = new_node(db);
		//printf("x:%d\n",x);
		db->data[x].c = c;
		//printf("add %c\n",c);
	}
	//printf("insert %s\n",s);
	//printf("d:  %d\n",d);
	if (c < db->data[x].c) {
		//printf("choice a\n");
		no = insert(db, db->data[x].left, s, value, d, len_of_s,
			    small_value_len);
		db->data[x].left = no;
	} else if (c > db->data[x].c) {
		//printf("choice b\n");
		no = insert(db, db->data[x].right, s, value, d, len_of_s,
			    small_value_len);
		db->data[x].right = no;
	} else if (d < len_of_s - 1) {

		//printf("choice c\n");
		no = insert(db, db->data[x].mid, s, value, d + 1, len_of_s,
			    small_value_len);
		db->data[x].mid = no;
	} else {

		//printf("choice d\n");
		db->data[x].value = value;
		db->data[x].small_value_len = small_value_len;
		//printf("==%llu\n",value);
	}
	return x;
}

static uint32 search(tst_db * db, uint32 node, const char *s, int d,
		     int len_of_s)
{
	tst_node *data = db->data;
	//printf("->%d,%s\n",node,s+d);
	if (node <= 0)
		return 0;
	char c = s[d];
	//printf("key: %s, d:%d\n",s,d);
	//printf("###%c,%c\n",c,data[node].c);
	//printf("~~~%d,%d\n",d,strlen(s));
	if (c < data[node].c)
		return search(db, data[node].left, s, d, len_of_s);
	else if (c > data[node].c)
		return search(db, data[node].right, s, d, len_of_s);
	else if (d < len_of_s - 1)
		return search(db, data[node].mid, s, d + 1, len_of_s);
	else
		return node;
}

static void put(tst_db * db, const char *key, uint64 value,
		char small_value_len)
{
	int len_of_key = strlen(key);
	if (len_of_key <= 0)
		return;
	db->root =
	    insert(db, db->root, key, value, 0, len_of_key,
		   small_value_len);
}


static uint32 get_node(tst_db * db, const char *key)
{
	int len_of_key = strlen(key);
	if (len_of_key <= 0)
		return 0;
	uint32 node = search(db, db->root, key, 0, len_of_key);
	return node;
}

tst_db *tst_open(const char *full_file_name)
{
	FILE *file_ptr = fopen(full_file_name, "rb");
	tst_db *db;
	int tmp_len_of_key, ct_read = 0, ct_record = 0;
	int tmp_len_of_value;
	char key_buf[256];
	int check_number;

	uint64 tmp_cur, tmp_small_value;

	if (file_ptr == NULL) {	//not exists;
		file_ptr = fopen(full_file_name, "wb");
		if (file_ptr == NULL) {
			printf("the path of %s not exists\n",
			       full_file_name);
			exit(1);
		} else {
			fclose(file_ptr);
			db = create_tst_db();
			db->read_file_ptr = fopen(full_file_name, "rb");
			db->write_file_ptr = fopen(full_file_name, "ab");
			check_number = MAGIC_NUMBER;
			fwrite(&check_number,sizeof(int),1,db->write_file_ptr);
			fflush(db->write_file_ptr);
		}
	} else {
		db = create_tst_db();
		db->read_file_ptr = fopen(full_file_name, "rb");
		db->write_file_ptr = fopen(full_file_name, "ab");
	}
		
	printf(">> db loading...\n");
	fseek(db->read_file_ptr, 0, SEEK_SET);
	fread(&check_number,sizeof(int),1,db->read_file_ptr);
	if(check_number!=MAGIC_NUMBER){
		printf("%s is not a valid tstdb file\n",full_file_name);
		exit(1);
	}
	fseek(db->read_file_ptr,4,SEEK_SET);

	while (1) {
		tmp_cur = ftell(db->read_file_ptr);
		ct_read =
		    fread(&tmp_len_of_key, sizeof(int), 1,
			  db->read_file_ptr);
		if (ct_read <= 0) {
			break;
		}
		fread(&tmp_len_of_value, sizeof(int), 1,
		      db->read_file_ptr);
		//printf("key len:%d\n",tmp_len_of_key);
		//printf("value len:%d\n",tmp_len_of_value);
		fread(key_buf, 1, tmp_len_of_key, db->read_file_ptr);
		key_buf[tmp_len_of_key] = '\0';
		if (tmp_len_of_value > sizeof(uint64)) {
			fseek(db->read_file_ptr, tmp_len_of_value,
			      SEEK_CUR);
			//printf("tmp_cur: %d\n",tmp_cur);
			put(db, key_buf, tmp_cur, 0);
		} else {

			tmp_small_value = 0;
			fread(&tmp_small_value, 1, tmp_len_of_value,
			      db->read_file_ptr);
			put(db, key_buf, tmp_small_value,
			    (char) tmp_len_of_value);
		}
		ct_record++;
		if (ct_record % 20000 == 0) {
			printf(">> %d records loaded\n", ct_record);
		}
		//printf("put cur %s,%lu\n",key_buf,tmp_cur);
	}
	db->ready = 1;
	printf(">> db ready!\n");
	return db;
}


void tst_close(tst_db * db)
{
	db->ready = 0;
	fflush(db->write_file_ptr);
	fclose(db->write_file_ptr);
	fclose(db->read_file_ptr);
	free_tst_db(db);
}

void tst_put(tst_db * db, const char *key, const char *value,
	     uint32 len_of_value)
{
	uint64 old_cur = ftell(db->write_file_ptr);
	int len_of_key = strlen(key);
	uint64 tmp_value;
	if (len_of_key <= 0 || len_of_key > 255)
		return;
	if (db->ready != 1)
		return;
	if (db->write_file_ptr) {
		fwrite(&len_of_key, sizeof(int), 1, db->write_file_ptr);
		fwrite(&len_of_value, sizeof(int), 1, db->write_file_ptr);
		fwrite(key, 1, len_of_key, db->write_file_ptr);
		fwrite(value, 1, len_of_value, db->write_file_ptr);
		if (len_of_value > sizeof(uint64)) {
			put(db, key, old_cur, 0);
		} else {
			tmp_value = 0;
			memcpy(&tmp_value, value, len_of_value);
			put(db, key, tmp_value, (char) len_of_value);
		}
	}
}

void tst_get(tst_db * db, const char *key, char *value,
	     uint32 * len_of_value_ptr)
{
	uint32 node = get_node(db, key);
	int len_of_key;
	int len_of_value;
	uint64 cur;
	//printf("try to find key: %s,len:%d\n",key,strlen(key));
	//printf("small value %d\n",db->data[node].small_value_len);
	//printf("node value %llu\n",db->data[node].value);
	if (db->ready != 1)
		return;
	ftell(db->write_file_ptr);
	if (node == 0) {	//not exists
		*len_of_value_ptr = 0;
	} else {
		if (db->data[node].small_value_len == 0
		    && db->data[node].value != 0) {
			cur = db->data[node].value;
			fseek(db->read_file_ptr, cur, SEEK_SET);
			fread(&len_of_key, sizeof(int), 1,
			      db->read_file_ptr);
			fread(&len_of_value, sizeof(int), 1,
			      db->read_file_ptr);
			fseek(db->read_file_ptr, len_of_key, SEEK_CUR);
			fread(value, 1, len_of_value, db->read_file_ptr);
			*len_of_value_ptr = len_of_value;
		} else {
			*len_of_value_ptr =
			    (uint32) db->data[node].small_value_len;
			memcpy(value, (char *) &(db->data[node].value),
			       *len_of_value_ptr);
		}
	}
}
