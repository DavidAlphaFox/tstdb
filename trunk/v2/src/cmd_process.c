#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "buffer_pool.h"
#include "tstserver.h"
#include "cmd_process.h"
#include "tst.h"
#define VALUE_BUF_SIZE 2000000

extern FILE *g_data_file_r[]; //for reading
extern FILE *g_data_file_w; //for writing
extern FILE *g_binlog_file;
extern pthread_rwlock_t g_reader_lock;
extern pthread_mutex_t g_writer_lock;

extern tst_db* g_tst;
extern void append_send_data(struct io_data_t *p, const char *data, int data_len);

char g_value_buf[WORKER_COUNT][VALUE_BUF_SIZE];

static int ends_with(const char* s1, const char* s2)
{
	int len_s2 = strlen(s2);
	int len_s1 = strlen(s1);
	if(len_s2 > len_s1)
		return 0;
	if(memcmp(s1+len_s1-len_s2, s2, len_s2)==0)
		return 1;
	else
		return 0;			
}

void cmd_do_get(struct io_data_t* p, const char* header )
{
	char key[MAX_KEY_SIZE]={0};
	int tmp[3],body_len,flag,expire;
	int total =0;
	const char* ptr = header;
	int span;
	uint64 value_offset;

	ptr += strcspn(ptr," ");
	ptr++; //skip method
	//printf("==============%d\n",WORKER_COUNT);

	while(1){	
			span = strcspn(ptr," \0");
			memcpy(key,ptr,span);
			key[span]='\0';

			pthread_rwlock_rdlock(& g_reader_lock);
			value_offset = tst_get(g_tst, key);
			pthread_rwlock_unlock(& g_reader_lock);

			g_value_buf[p->worker_no][total]='\0';
			if(value_offset>0){
					fseek(g_data_file_r[p->worker_no], value_offset, SEEK_SET);

					fread(tmp,sizeof(int),3, g_data_file_r[p->worker_no]);
					body_len = tmp[0];
					flag =  tmp[1];
					expire = tmp[2];
					if(body_len>0){	
							total += snprintf(g_value_buf[p->worker_no]+total,VALUE_BUF_SIZE,"VALUE %s %d %d\r\n",
											key, flag, body_len); 	
							fread(g_value_buf[p->worker_no]+total,sizeof(char),body_len,g_data_file_r[p->worker_no]);

							total+= body_len;
							memcpy(g_value_buf[p->worker_no]+total,"\r\n",strlen("\r\n"));	
							total+=strlen("\r\n");
					}
			}
			ptr+=span;
			if(*ptr =='\0') break;
			ptr++; //next key
	}
	memcpy(g_value_buf[p->worker_no]+total,"END\r\n",strlen("END\r\n"));
	append_send_data(p, g_value_buf[p->worker_no], total+strlen("END\r\n"));
}

void cmd_do_set(struct io_data_t* p, const char* header, const char* body )
{
	char *msg="STORED\r\n";
	int flag,expire,body_len;
	int tmp[3];
	char key[MAX_KEY_SIZE]={0};
	char method[256]={0};
	if(sscanf(header,"%s %s %d %d %d",method, key,&flag,&expire,&body_len)<5)
		return;

	pthread_mutex_lock(&g_writer_lock);
	//write data file	
	uint64 value_offset = ftell(g_data_file_w);	
	tmp[0]=body_len;
	tmp[1]=flag;
	tmp[2]=expire;
	fwrite(tmp,sizeof(int),3,g_data_file_w);	
	fwrite(body,sizeof(char),body_len, g_data_file_w);
	fflush(g_data_file_w);

	//write binlog file
	int key_len = strlen(key);
	fwrite(&key_len,sizeof(int),1,g_binlog_file);
	fwrite(key,sizeof(char), key_len, g_binlog_file);
	fwrite(&value_offset,sizeof(uint64),1,g_binlog_file);
	fflush(g_binlog_file);
	pthread_mutex_unlock(&g_writer_lock);

	//change tst in memory
	pthread_rwlock_wrlock(&g_reader_lock);
	tst_put(g_tst,key, value_offset);
 	pthread_rwlock_unlock(&g_reader_lock);
		
	if(!ends_with(header," noreply")){
		append_send_data(p,msg,strlen(msg) );
	}
}

void cmd_do_delete(struct io_data_t* p, const char* header )
{
	char *msg="DELETED\r\n";
	char key[MAX_KEY_SIZE]={0};
	char method[256]={0};
	if(sscanf(header,"%s %s",method, key)!=2)
		return;
	int key_len = strlen(key);
	uint64 delete_flag=0;

	pthread_mutex_lock(&g_writer_lock);
	fwrite(&key_len,sizeof(int),1,g_binlog_file);
	fwrite(key,sizeof(char), key_len, g_binlog_file);
	fwrite(&delete_flag,sizeof(uint64),1,g_binlog_file);
	fflush(g_binlog_file);
	pthread_mutex_unlock(&g_writer_lock);

	
	pthread_rwlock_wrlock(&g_reader_lock);
	tst_delete(g_tst, key);
	pthread_rwlock_unlock(&g_reader_lock);

	append_send_data(p,msg,strlen(msg));
}


