#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer_pool.h"
#include "tstserver.h"
#include "cmd_process.h"


extern FILE *g_data_file;
extern FILE *g_index_file;
extern void append_send_data(struct io_data_t *p, const char *data, int data_len);


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
	char *msg="OK\r\n";
	append_send_data(p,msg,strlen(msg));
}

void cmd_do_set(struct io_data_t* p, const char* header, const char* body )
{
	char *msg="STORED\r\n";
	if(!ends_with(header," noreply")){
		append_send_data(p,msg,strlen(msg) );
	}
}

void cmd_do_delete(struct io_data_t* p, const char* header )
{
	char *msg="DELETED\r\n";
	append_send_data(p,msg,strlen(msg));
}


