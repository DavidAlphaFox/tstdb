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

#define HEADER_BUF_SIZE 512
#define VALUE_BUF_SIZE 2000000
#define MAX_PREFIX_RESULT 10000
#define PREFIX_BUF_SIZE 2560000

extern FILE *g_data_file_r[];	//for reading
extern FILE *g_data_file_w;	//for writing
extern FILE *g_binlog_file;
extern pthread_rwlock_t g_reader_lock;
extern pthread_mutex_t g_writer_lock;

extern tst_db *g_tst;
extern void append_send_data(struct io_data_t *p, const char *data, int data_len);

char g_value_buf[WORKER_COUNT][VALUE_BUF_SIZE];
char g_prefix_buf[WORKER_COUNT][PREFIX_BUF_SIZE];

static int
ends_with(const char *s1, const char *s2)
{
	int len_s2 = strlen(s2);
	int len_s1 = strlen(s1);
	if (len_s2 > len_s1)
		return 0;
	if (memcmp(s1 + len_s1 - len_s2, s2, len_s2) == 0)
		return 1;
	else
		return 0;
}

void
cmd_do_prefix(struct io_data_t *p, const char *header)
{
	char prefix[MAX_KEY_SIZE] = { 0 };
	int limit;
	int result_size = 0;
	int result_bytes_len = 0;
	char rsps_header[HEADER_BUF_SIZE] = { 0 };
	char *result = g_prefix_buf[p->worker_no];
	char sorting_order[10] = { 0 };
	enum SortingOrder e_sorting_order;
	result[0] = '\0';

	if (sscanf(header, "prefix %s %d %s", prefix, &limit, sorting_order) == 3) {
		if (limit > MAX_PREFIX_RESULT)
			limit = MAX_PREFIX_RESULT;
		if (strcmp(sorting_order, "asc") == 0) {
			e_sorting_order = ASC;
		} else {
			e_sorting_order = DESC;
		}
		pthread_rwlock_rdlock(&g_reader_lock);
		tst_prefix(g_tst, prefix, result, &result_size, limit, e_sorting_order);
		pthread_rwlock_unlock(&g_reader_lock);

		result_bytes_len = strlen(result);
		snprintf(rsps_header, HEADER_BUF_SIZE, "KEYS %d %d\r\n", result_size, result_bytes_len);
		append_send_data(p, rsps_header, strlen(rsps_header));
		append_send_data(p, result, result_bytes_len);
		append_send_data(p, "END\r\n", strlen("END\r\n"));

	} else
		append_send_data(p, "ERROR\r\n", strlen("ERROR\r\n"));
}

void
cmd_do_less(struct io_data_t *p, const char *header)
{
	char prefix[MAX_KEY_SIZE] = { 0 };
	int limit;
	int result_size = 0;
	int result_bytes_len = 0;
	char rsps_header[HEADER_BUF_SIZE] = { 0 };
	char *result = g_prefix_buf[p->worker_no];
	result[0] = '\0';

	if (sscanf(header, "less %s %d", prefix, &limit) == 2) {
		if (limit > MAX_PREFIX_RESULT)
			limit = MAX_PREFIX_RESULT;

		pthread_rwlock_rdlock(&g_reader_lock);
		tst_less(g_tst, prefix, result, &result_size, limit);
		pthread_rwlock_unlock(&g_reader_lock);

		result_bytes_len = strlen(result);
		snprintf(rsps_header, HEADER_BUF_SIZE, "KEYS %d %d\r\n", result_size, result_bytes_len);
		append_send_data(p, rsps_header, strlen(rsps_header));
		append_send_data(p, result, result_bytes_len);
		append_send_data(p, "END\r\n", strlen("END\r\n"));

	} else
		append_send_data(p, "ERROR\r\n", strlen("ERROR\r\n"));
}

void
cmd_do_greater(struct io_data_t *p, const char *header)
{
	char prefix[MAX_KEY_SIZE] = { 0 };
	int limit;
	int result_size = 0;
	int result_bytes_len = 0;
	char rsps_header[HEADER_BUF_SIZE] = { 0 };
	char *result = g_prefix_buf[p->worker_no];
	result[0] = '\0';

	if (sscanf(header, "greater %s %d", prefix, &limit) == 2) {
		if (limit > MAX_PREFIX_RESULT)
			limit = MAX_PREFIX_RESULT;

		pthread_rwlock_rdlock(&g_reader_lock);
		tst_greater(g_tst, prefix, result, &result_size, limit);
		pthread_rwlock_unlock(&g_reader_lock);

		result_bytes_len = strlen(result);
		snprintf(rsps_header, HEADER_BUF_SIZE, "KEYS %d %d\r\n", result_size, result_bytes_len);
		append_send_data(p, rsps_header, strlen(rsps_header));
		append_send_data(p, result, result_bytes_len);
		append_send_data(p, "END\r\n", strlen("END\r\n"));

	} else
		append_send_data(p, "ERROR\r\n", strlen("ERROR\r\n"));
}

static void
delete_expire_value(char* key)
{
	int key_len = strlen(key);
	uint64 delete_flag = 0;

	pthread_mutex_lock(&g_writer_lock);
	fwrite(&key_len, sizeof(int), 1, g_binlog_file);
	fwrite(key, sizeof(char), key_len, g_binlog_file);
	fwrite(&delete_flag, sizeof(uint64), 1, g_binlog_file);
	fflush(g_binlog_file);

	pthread_rwlock_wrlock(&g_reader_lock);
	tst_delete(g_tst, key);
	pthread_rwlock_unlock(&g_reader_lock);

	pthread_mutex_unlock(&g_writer_lock);
}

void
cmd_do_get(struct io_data_t *p, const char *header, int r_sign)
{
	char key[MAX_KEY_SIZE] = { 0 };
	int tmp[3], body_len, flag, expire;
	int total = 0;
	const char *ptr = header;
	int span,current_time;
	uint64 value_offset;

	ptr += strcspn(ptr, " ");
	ptr++;			//skip method
	//printf("==============%d\n",WORKER_COUNT);

	while (1) {
		span = strcspn(ptr, " \0");
		memcpy(key, ptr, span);
		key[span] = '\0';

		pthread_rwlock_rdlock(&g_reader_lock);
		value_offset = tst_get(g_tst, key);
		pthread_rwlock_unlock(&g_reader_lock);

		g_value_buf[p->worker_no][total] = '\0';
		if (value_offset > 0) {
			fseek(g_data_file_r[p->worker_no], value_offset, SEEK_SET);

			fread(tmp, sizeof(int), 3, g_data_file_r[p->worker_no]);
			body_len = tmp[0];
			flag = tmp[1];
			expire = tmp[2];
			if(expire > 0){
				current_time = (int)time(NULL);
				if(expire <= current_time){ //value expired
					delete_expire_value(key);
					body_len = -1 ;//to walk around nex if
				}
			} 
			if (body_len >= 0) {
				if (r_sign == 0) {
					total += snprintf(g_value_buf[p->worker_no] + total, VALUE_BUF_SIZE, "VALUE %s %d %d\r\n", key, flag, body_len);
				} else {
					total += snprintf(g_value_buf[p->worker_no] + total, VALUE_BUF_SIZE, "VALUE %s %d %d %llu\r\n",
							  key, flag, body_len, value_offset);

				}
				fread(g_value_buf[p->worker_no] + total, sizeof(char), body_len, g_data_file_r[p->worker_no]);

				total += body_len;
				memcpy(g_value_buf[p->worker_no] + total, "\r\n", strlen("\r\n"));
				total += strlen("\r\n");
			}
		}
		ptr += span;
		if (*ptr == '\0')
			break;
		ptr++;		//next key
	}
	memcpy(g_value_buf[p->worker_no] + total, "END\r\n", strlen("END\r\n"));
	append_send_data(p, g_value_buf[p->worker_no], total + strlen("END\r\n"));
}

//compare and swap
//use 'gets key1' to fetch the sign of key
void
cmd_do_cas(struct io_data_t *p, const char *header, const char *body)
{
	char *msg_ok = "STORED\r\n";
	char *msg_fail = "EXISTS\r\n";
	int flag, expire, body_len;
	int tmp[3];
	char key[MAX_KEY_SIZE] = { 0 };
	char method[HEADER_BUF_SIZE] = { 0 };
	char *msg;
	uint64 sign, value_offset;
	if (sscanf(header, "%s %s %d %d %d %llu", method, key, &flag, &expire, &body_len, &sign) < 6)
		return;
	if (body_len < 0)
		return;
	pthread_mutex_lock(&g_writer_lock);
	do {
		value_offset = tst_get(g_tst, key);
		if (value_offset != sign) {
			msg = msg_fail;
			break;
		}
		//write data file       
		value_offset = ftell(g_data_file_w);
		tmp[0] = body_len;
		tmp[1] = flag;
		tmp[2] = expire;
		fwrite(tmp, sizeof(int), 3, g_data_file_w);
		fwrite(body, sizeof(char), body_len, g_data_file_w);
		fflush(g_data_file_w);

		//write binlog file
		int key_len = strlen(key);
		fwrite(&key_len, sizeof(int), 1, g_binlog_file);
		fwrite(key, sizeof(char), key_len, g_binlog_file);
		fwrite(&value_offset, sizeof(uint64), 1, g_binlog_file);
		fflush(g_binlog_file);

		//change tst in memory
		pthread_rwlock_wrlock(&g_reader_lock);
		tst_put(g_tst, key, value_offset);
		pthread_rwlock_unlock(&g_reader_lock);

		msg = msg_ok;
	} while (0);

	pthread_mutex_unlock(&g_writer_lock);

	if (!ends_with(header, " noreply")) {
		append_send_data(p, msg, strlen(msg));
	}

}

void
cmd_do_set(struct io_data_t *p, const char *header, const char *body)
{
	char *msg = "STORED\r\n";
	int flag, expire, body_len;
	int tmp[3];
	char key[MAX_KEY_SIZE] = { 0 };
	char method[HEADER_BUF_SIZE] = { 0 };
	if (sscanf(header, "%s %s %d %d %d", method, key, &flag, &expire, &body_len) < 5)
		return;
	if (body_len < 0)
		return;
	pthread_mutex_lock(&g_writer_lock);
	//write data file       
	uint64 value_offset = ftell(g_data_file_w);
	tmp[0] = body_len;
	tmp[1] = flag;
	tmp[2] = expire;
	if(expire>0){ //value may expire
		tmp[2] += time(NULL);
	}
	fwrite(tmp, sizeof(int), 3, g_data_file_w);
	fwrite(body, sizeof(char), body_len, g_data_file_w);
	fflush(g_data_file_w);

	//write binlog file
	int key_len = strlen(key);
	fwrite(&key_len, sizeof(int), 1, g_binlog_file);
	fwrite(key, sizeof(char), key_len, g_binlog_file);
	fwrite(&value_offset, sizeof(uint64), 1, g_binlog_file);
	fflush(g_binlog_file);

	//change tst in memory
	pthread_rwlock_wrlock(&g_reader_lock);
	tst_put(g_tst, key, value_offset);
	pthread_rwlock_unlock(&g_reader_lock);

	pthread_mutex_unlock(&g_writer_lock);

	if (!ends_with(header, " noreply")) {
		append_send_data(p, msg, strlen(msg));
	}
}



void
cmd_do_delete(struct io_data_t *p, const char *header)
{
	char *msg_ok = "DELETED\r\n";
	char *msg_fail = "NOT_FOUND\r\n";
	char *msg;
	char key[MAX_KEY_SIZE] = { 0 };
	char method[HEADER_BUF_SIZE] = { 0 };
	if (sscanf(header, "%s %s", method, key) != 2)
		return;
	int key_len = strlen(key);
	uint64 delete_flag = 0;

	pthread_mutex_lock(&g_writer_lock);
	do {
		if (tst_get(g_tst, key) == 0) {
			msg = msg_fail;
			break;
		}
		fwrite(&key_len, sizeof(int), 1, g_binlog_file);
		fwrite(key, sizeof(char), key_len, g_binlog_file);
		fwrite(&delete_flag, sizeof(uint64), 1, g_binlog_file);
		fflush(g_binlog_file);

		pthread_rwlock_wrlock(&g_reader_lock);
		tst_delete(g_tst, key);
		pthread_rwlock_unlock(&g_reader_lock);
		msg = msg_ok;
	} while (0);

	pthread_mutex_unlock(&g_writer_lock);
	append_send_data(p, msg, strlen(msg));
}

static void
incr_or_decr(struct io_data_t *p, const char *header, int is_decr)
{
	char *msg_fail = "NOT_FOUND\r\n";
	char *msg;
	char key[MAX_KEY_SIZE] = { 0 };
	char method[HEADER_BUF_SIZE] = { 0 };
	char str_number[HEADER_BUF_SIZE] = { 0 };
	int number = 0, delta = 0;
	uint64 value_offset = 0;
	int body_len, flag, expire, tmp[3], key_len;

	if (sscanf(header, "%s %s %d", method, key, &delta) != 3)
		return;

	pthread_mutex_lock(&g_writer_lock);
	do {
		if ((value_offset = tst_get(g_tst, key)) == 0) {
			msg = msg_fail;
			break;
		}
		fseek(g_data_file_r[p->worker_no], value_offset, SEEK_SET);
		fread(tmp, sizeof(int), 3, g_data_file_r[p->worker_no]);
		body_len = tmp[0];
		flag = tmp[1];
		expire = tmp[2];
		fread(str_number, sizeof(char), body_len, g_data_file_r[p->worker_no]);
		number = atoi(str_number);
		if (is_decr)
			number -= delta;
		else
			number += delta;

		snprintf(str_number, HEADER_BUF_SIZE, "%d", number);
		//write data file       
		uint64 value_offset = ftell(g_data_file_w);
		tmp[0] = strlen(str_number);
		tmp[1] = flag;
		tmp[2] = expire;
		fwrite(tmp, sizeof(int), 3, g_data_file_w);
		fwrite(str_number, sizeof(char), tmp[0], g_data_file_w);
		fflush(g_data_file_w);

		//write binlog file
		key_len = strlen(key);
		fwrite(&key_len, sizeof(int), 1, g_binlog_file);
		fwrite(key, sizeof(char), key_len, g_binlog_file);
		fwrite(&value_offset, sizeof(uint64), 1, g_binlog_file);
		fflush(g_binlog_file);

		//change tst in memory
		pthread_rwlock_wrlock(&g_reader_lock);
		tst_put(g_tst, key, value_offset);
		pthread_rwlock_unlock(&g_reader_lock);

		strcat(str_number, "\r\n");
		msg = str_number;
	} while (0);

	pthread_mutex_unlock(&g_writer_lock);
	append_send_data(p, msg, strlen(msg));
}

void
cmd_do_incr(struct io_data_t *p, const char *header)
{
	incr_or_decr(p, header, 0);
}

void
cmd_do_decr(struct io_data_t *p, const char *header)
{
	incr_or_decr(p, header, 1);
}
