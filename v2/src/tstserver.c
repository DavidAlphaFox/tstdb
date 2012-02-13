#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>

#include "buffer_pool.h"
#include "tstserver.h"
#include "tst.h"
#include "cmd_process.h"

#define MAX_EPOLL_FD 40960
#define WORKER_COUNT 2
#define BUF_POOL_SIZE 50000
#define MAX_BODY_SIZE 1000000

int g_ep_fd[WORKER_COUNT], listen_fd;
int g_delay;
int g_shutdown_flag;
int g_nolog;
FILE *g_logger;
bp_t *g_bufpoll[WORKER_COUNT];
FILE *g_data_file, *g_index_file;

struct io_data_t g_io_table[WORKER_COUNT][MAX_EPOLL_FD];

static void
tstserver_log(const char *fmt, ...)
{
	if (0 == g_nolog) {
		char timestamp[64];
		char msg[4096] = { 0 };
		time_t now = time(NULL);
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(msg, sizeof(msg), fmt, ap);
		va_end(ap);
		strftime(timestamp, sizeof(timestamp), "%d %b %H:%M:%S", localtime(&now));
		fprintf(g_logger, "[%d] %s %s\n", (int)getpid(), timestamp, msg);
		fflush(g_logger);
	}
}


static void *handle_core(void *param);


static void
setnonblocking(int fd)
{
	int opts;
	opts = fcntl(fd, F_GETFL);
	if (opts < 0) {
		fprintf(stderr, "fcntl failed\n");
		return;
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(fd, F_SETFL, opts) < 0) {
		fprintf(stderr, "fcntl failed\n");
		return;
	}
	return;
}

static void
usage()
{
	printf("usage:  tstserver \n" "-p <port> \n" "-f <data file>\n" "-g <log file>\n" "-h (show help)\n" "-q (no log mode)\n");
}

static struct io_data_t *
alloc_io_data(int client_fd, struct sockaddr_in *client_addr, int worker_no)
{
	struct io_data_t *io_data_ptr = &g_io_table[worker_no][client_fd];

	io_data_ptr->fd = client_fd;
	io_data_ptr->worker_no = worker_no;
	if (client_addr)
		io_data_ptr->addr = *client_addr;
	io_data_ptr->in_buf_head = NULL;
	io_data_ptr->in_buf_tail = NULL;
	io_data_ptr->out_buf_head = NULL;
	io_data_ptr->out_buf_tail = NULL;
	io_data_ptr->header_len = 0;
	io_data_ptr->body_len = -1;
	return io_data_ptr;
}


static void
free_in_buf_list(struct io_data_t *p)
{
	buffer_t *q = p->in_buf_head, *tmp;
	bp_t *mybp = g_bufpoll[p->worker_no];
	while (q) {
		tmp = q;
		if (q->next >= 0) {
			q = mybp->base + q->next;
		} else {
			q = NULL;
		}
		free_buf(mybp, tmp);
	}
	p->in_buf_head = NULL;
	p->in_buf_len = 0;
}

static void
free_out_buf_list(struct io_data_t *p)
{
	buffer_t *q = p->out_buf_head, *tmp;
	bp_t *mybp = g_bufpoll[p->worker_no];
	while (q) {
		tmp = q;
		if (q->next >= 0) {
			q = mybp->base + q->next;
		} else {
			q = NULL;
		}
		free_buf(mybp, tmp);
	}
	p->out_buf_head = NULL;
	p->out_buf_len = 0;
}

static void
destroy_io_data(struct io_data_t *io_data_ptr)
{
	if (NULL == io_data_ptr)
		return;
	if (io_data_ptr->in_buf_head)
		free_in_buf_list(io_data_ptr);
	if (io_data_ptr->out_buf_head)
		free_out_buf_list(io_data_ptr);

	io_data_ptr->in_buf_head = NULL;
	io_data_ptr->out_buf_head = NULL;
	io_data_ptr->in_buf_tail = NULL;
	io_data_ptr->out_buf_tail = NULL;
}

static void
exit_hook(int number)
{
	close(listen_fd);
	g_shutdown_flag = 1;
	tstserver_log(">> [%d]will shutdown...", getpid());
}

static void
parse_args(int argc, char *argv[], char **ip_binding, int *port_listening, char **data_file, char **log_file)
{
	int opt;
	while ((opt = getopt(argc, argv, "l:p:f:g:hq")) != -1) {
		switch (opt) {
			case 'l':
				*ip_binding = strdup(optarg);
				break;
			case 'p':
				*port_listening = atoi(optarg);
				if (*port_listening <= 0) {
					printf(">> invalid port : %s\n", optarg);
					exit(1);
				}
				break;
			case 'g':
				*log_file = strdup(optarg);
				break;
			case 'f':
				*data_file = strdup(optarg);
				break;
			case 'q':
				g_nolog = 1;
				break;
			case 'h':
				usage();
				exit(0);
		}

	}
}

static void
do_bind_and_listen(int port_listening, const char *ip_binding)
{
	struct sockaddr_in server_addr;
	int on = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, (int[]) {
		   1}, sizeof(int));
	setsockopt(listen_fd, IPPROTO_TCP, TCP_QUICKACK, (int[]) {
		   1}, sizeof(int));

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((short)port_listening);
	server_addr.sin_addr.s_addr = inet_addr(ip_binding);

	if (-1 == bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		perror("bind error");
		exit(-1);
	}

	if (-1 == listen(listen_fd, 64)) {
		perror("listen error");
		exit(-1);
	}

	setnonblocking(listen_fd);
}

static void
do_accept()
{
	fd_set rset, eset;
	int client_fd = 0, select_ret = 0;
	struct sockaddr_in client_addr;
	register unsigned int worker_pointer = 0;
	char ip_buf[256] = { 0 };
	socklen_t client_n;
	struct epoll_event ev;

	bzero(&client_addr, sizeof(client_addr));
	bzero(&client_n, sizeof(client_n));
	FD_ZERO(&rset);
	FD_ZERO(&eset);
	FD_SET(listen_fd, &rset);

	while (1) {
		select_ret = select(listen_fd + 1, &rset, NULL, &eset, NULL);
		if (select_ret > 0 && FD_ISSET(listen_fd, &rset)) {
			if ((client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_n)) > 0) {
				//printf("%u\n",client_addr.sin_addr.s_addr);
				setnonblocking(client_fd);
				ev.data.ptr = alloc_io_data(client_fd, (struct sockaddr_in *)&client_addr, worker_pointer);
				ev.events = EPOLLIN;
				epoll_ctl(g_ep_fd[worker_pointer++], EPOLL_CTL_ADD, client_fd, &ev);
				inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
				tstserver_log("[CONN]Connection from %s", ip_buf);
				if (worker_pointer == WORKER_COUNT)
					worker_pointer = 0;
			} else {
				perror("please check ulimit -n");
			}
		} else if (errno == EBADF && g_shutdown_flag) {
			break;
		} else {
			if (0 == g_shutdown_flag) {
				perror("select error");
			}
		}
	}
}

static void init_data(const char* data_file_name)
{
	char index_file_name[256];
	snprintf(index_file_name, 256, "%s.index",data_file_name);
	g_data_file = fopen(data_file_name,"a+");
	g_index_file = fopen(index_file_name,"a+");
	assert(g_data_file>0);
	assert(g_index_file>0);
}

int
main(int argc, char **argv)
{
	char *ip_binding = "0.0.0.0";
	int port_listening = 8402;
	char *data_file = "";
	char *log_file = "";

	pthread_t tid[WORKER_COUNT];
	pthread_attr_t tattr[WORKER_COUNT];
	struct thread_data_t tdata[WORKER_COUNT];

	register unsigned int i;

	g_delay = 0;
	g_shutdown_flag = 0;
	g_nolog = 0;


	if (argc == 1) {
		usage();
		return 1;
	}

	parse_args(argc, argv, &ip_binding, &port_listening, &data_file, &log_file);

	g_logger = fopen(log_file, "a");
	if (g_logger <= 0) {
		fprintf(stderr, "%s\n", log_file);
		perror("create log file failed.");
		exit(1);
	}

	tstserver_log(">> IP listening:%s", ip_binding);
	tstserver_log(">> port: %d", port_listening);
	tstserver_log(">> data_file: %s", data_file);
	tstserver_log(">> reponse delay(MS): %d", g_delay);
	tstserver_log(">> log file: %s", log_file);
	tstserver_log(">> no log: %d", g_nolog);


	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, exit_hook);
	signal(SIGQUIT, exit_hook);
	signal(SIGTERM, exit_hook);
	signal(SIGHUP, SIG_IGN);

	init_data(data_file);
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listen_fd) {
		perror("listen faild!");
		exit(-1);
	}

	do_bind_and_listen(port_listening, ip_binding);

	for (i = 0; i < WORKER_COUNT; i++) {
		g_ep_fd[i] = epoll_create(MAX_EPOLL_FD);
		if (g_ep_fd[i] < 0) {
			perror("epoll_create failed.");
			exit(-1);
		}
		g_bufpoll[i] = bp_new(BUF_POOL_SIZE);
	}

	for (i = 0; i < WORKER_COUNT; i++) {
		pthread_attr_init(tattr + i);
		pthread_attr_setdetachstate(tattr + i, PTHREAD_CREATE_JOINABLE);
		tdata[i].worker_no = i;
		if (pthread_create(tid + i, tattr + i, handle_core, tdata + i) != 0) {
			fprintf(stderr, "pthread_create failed\n");
			return -1;
		}

	}

	do_accept();


	for (i = 0; i < WORKER_COUNT; i++) {
		close(g_ep_fd[i]);
	}

	fclose(g_logger);
	fclose(g_data_file);
	fclose(g_index_file);

	tstserver_log(">> [%d]waiting worker thread....", getpid());

	for (i = 0; i < WORKER_COUNT; i++)
		pthread_join(tid[i], NULL);

	tstserver_log(">> [%d]Bye~", getpid());
	return 0;
}

static void
destroy_fd(int myg_ep_fd, int client_fd, struct io_data_t *data_ptr, int case_no)
{
	struct sockaddr_in s_addr = data_ptr->addr;
	char ip_buf[256] = { 0 };
	epoll_ctl(myg_ep_fd, EPOLL_CTL_DEL, client_fd, NULL);
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);

	inet_ntop(AF_INET, &s_addr.sin_addr, ip_buf, sizeof(ip_buf));
	tstserver_log("[BYE]: %s, %d", ip_buf, case_no);
	destroy_io_data(data_ptr);
}



static int
append_recv_data(struct io_data_t *p)
{
	buffer_t *a_buf;
	int ct;
	int fd = p->fd;
	bp_t *mybp = g_bufpoll[p->worker_no];
	if (p->in_buf_tail == NULL) {
		p->in_buf_tail = alloc_buf(mybp);
		p->in_buf_head = p->in_buf_tail;
	} else {
		a_buf = alloc_buf(mybp);
		p->in_buf_tail->next = a_buf - (mybp->base);
		p->in_buf_tail = a_buf;
	}
	ct = recv(fd, p->in_buf_tail->data, BUF_UNIT_SIZE, MSG_DONTWAIT);
	if (ct > 0) {
		p->in_buf_tail->size = ct;
		p->in_buf_tail->data[ct] = '\0';
		//printf("%s",p->in_buf_tail->data);
		p->in_buf_len += ct;
	}
	return ct;
}

void
append_send_data(struct io_data_t *p, const char *data, int data_len)
{
	const char *k = data;
	int t = data_len;
	int cap = 0;
	buffer_t *q = p->out_buf_tail;
	bp_t *mybp = g_bufpoll[p->worker_no];
	do {
		if (q == NULL) {
			q = alloc_buf(mybp);
			if (p->out_buf_head == NULL) {
				p->out_buf_head = q;
				p->out_buf_tail = q;
			} else {
				p->out_buf_tail->next = q - (mybp->base);
				p->out_buf_tail = q;
			}
		}
		cap = BUF_UNIT_SIZE - (q->size);
		if (cap >= t) {	//enough
			memcpy(q->data + q->size, k, t);
			q->size += t;
			break;
		} else {
			memcpy(q->data + q->size, k, cap);
			q->size += cap;
			k += cap;
			t -= cap;
			q = NULL;
		}

	} while (1);
}

static int
has_header(struct io_data_t *p)
{
	int found = 0;
	int hd_len = 0;
	struct buffer_t *q;
	char *pos;
	bp_t *bp = g_bufpoll[p->worker_no];
	if (p->in_buf_head == NULL)
		return 0;
	if (p->header_len > 0)
		return 1;
	q = p->in_buf_head;
	do {
		q->data[q->size] = '\0';
		if ((pos = strstr(q->data, "\n")) != NULL) {
			found = 1;
			hd_len += (pos - (q->data));
			if(hd_len>0)p->header_len = hd_len-1;
			break;
		}
		if (q->next >= 0) {
			hd_len += q->size;
			q = bp->base + q->next;
		} else {
			break;
		}
	} while (1);
	return found;
}

static void
get_body_len(struct io_data_t *p,char* s_head_buf)
{
	buffer_t *q;
	char *header_msg= s_head_buf;
	int start;
	int body_len = 0;
	bp_t *mybp;
	char key[1024] = { 0 };
	int flag, expire, value_len;

	mybp = g_bufpoll[p->worker_no];
	if (p->body_len >= 0)
		return;
	q = p->in_buf_head;
	start = 0;
	do {
		memcpy(header_msg + start, q->data, q->size);
		start += q->size;
		if (start >= p->header_len)
			break;
		if (q->next >= 0) {
			q = mybp->base + q->next;
		} else {
			break;
		}
	} while (1);
	if (start >= p->header_len) {
		header_msg[p->header_len] = '\0';
		if (sscanf(header_msg, "set %s %d %d %d", key, &flag, &expire, &value_len) == 4) {
			if(value_len<0 || value_len>=MAX_BODY_SIZE) value_len=0;
			body_len = value_len;
		}
		p->body_len = body_len;
	}
}

static int
has_body(struct io_data_t *p)
{
	assert(p != NULL);
	if (p->body_len < 0)
		return 0;
	return p->in_buf_len >= (p->header_len + 4 + p->body_len);	//4=sizeof("\r\n\")*2
}

static void
read_header(struct io_data_t *p, char *header)
{
	int k = p->header_len + 2;
	int start = 0;
	buffer_t *q = p->in_buf_head, *tmp;
	bp_t *mybp;
	mybp = g_bufpoll[p->worker_no];

	assert(p->header_len >= 0);
	assert(q != NULL);
	while (k >= q->size) {
		tmp = q;
		memcpy(header + start, q->data, q->size);
		k -= q->size;
		start += q->size;
		if (q->next >= 0) {
			p->in_buf_head = (mybp->base + q->next);
			q = mybp->base + q->next;
			free_buf(mybp, tmp);
		} else {
			p->in_buf_head = NULL;
			p->in_buf_tail = NULL;
			q = NULL;
			free_buf(mybp, tmp);
			break;
		}
	}
	if (q != NULL) {
		memcpy(header + start, q->data, k);
		memcpy(q->data, q->data + k, q->size - k);
		q->size -= k;
		if (q->size == 0) {
			tmp = q;
			if (q->next >= 0) {
				p->in_buf_head = mybp->base + q->next;
			} else {
				p->in_buf_head = NULL;
				p->in_buf_tail = NULL;
			}
			free_buf(mybp, tmp);
		}
	}
	header[p->header_len] = '\0';
}

static void
read_body(struct io_data_t *p, char *body)
{
	int k = p->body_len + 2;
	int start = 0;

	buffer_t *q = p->in_buf_head, *tmp;
	bp_t *mybp;
	mybp = g_bufpoll[p->worker_no];

	assert(p->body_len >= 0);
	assert(q != NULL);
	while (k >= q->size) {
		tmp = q;
		memcpy(body + start, q->data, q->size);
		k -= q->size;
		start += q->size;
		if (q->next >= 0) {
			p->in_buf_head = (mybp->base + q->next);
			q = mybp->base + q->next;
			free_buf(mybp, tmp);
		} else {
			p->in_buf_head = NULL;
			p->in_buf_tail = NULL;
			q = NULL;
			free_buf(mybp, tmp);
			break;
		}
	}
	if (q != NULL) {
		memcpy(body + start, q->data, k);
		memcpy(q->data, q->data + k, q->size - k);
		q->size -= k;
		if (q->size == 0) {
			tmp = q;
			if (q->next >= 0) {
				p->in_buf_head = mybp->base + q->next;
			} else {
				p->in_buf_head = NULL;
				p->in_buf_tail = NULL;
			}
			free_buf(mybp, tmp);
		}
	}
	body[p->body_len] = '\0';
}

static int starts_with(const char* s1, const char* s2)
{
	int len_s2 = strlen(s2);
	if(memcmp(s1,s2,len_s2)==0)
		return 1;
	else
		return 0;
}

static void
handle_cmd(struct io_data_t *p, char *header, char *body)
{
	header[p->header_len]='\0';
	body[p->body_len]='\0';
	printf("header:%s\n", header);
	printf("body:%s\n", body);

	if (starts_with(header, "set")) {
		cmd_do_set(p,header,body);	
	}
	else if(starts_with(header,"get") ){
		cmd_do_get(p,header);
	}
	else if(starts_with(header,"delete")){
		cmd_do_delete(p,header);
	}
}


static void
fix_buf_len(struct io_data_t *p)
{
	if (p->body_len > 0) {
		p->in_buf_len -= (p->header_len + p->body_len + 4);
	} else {
		p->in_buf_len -= (p->header_len + p->body_len + 2);
	}
	if(p->in_buf_len<0) p->in_buf_len=0;
	p->header_len = 0;
	p->body_len = -1;
}

static void
handle_input(int worker_no, struct io_data_t *client_io_ptr, char *s_body_buf,char* s_head_buf)
{
	struct epoll_event ev;
	int fd = client_io_ptr->fd;
	char *header = s_head_buf;
	char *body = s_body_buf;

	int can_output = 0;
	if (append_recv_data(client_io_ptr) <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		destroy_fd(g_ep_fd[worker_no], fd, client_io_ptr, 2);
		return;
	}

	while (has_header(client_io_ptr)) {
		get_body_len(client_io_ptr,header);

		if (client_io_ptr->body_len == 0 || has_body(client_io_ptr)) {
			read_header(client_io_ptr, header);
			if (client_io_ptr->body_len > 0)
				read_body(client_io_ptr, body);
			handle_cmd(client_io_ptr, header, body);
			fix_buf_len(client_io_ptr);
			if (client_io_ptr->out_buf_head)
				can_output = 1;
		} else {
			break;
		}
	}

	if (can_output) {
		ev.events = EPOLLOUT | EPOLLIN;
		ev.data.ptr = client_io_ptr;
		epoll_ctl(g_ep_fd[worker_no], EPOLL_CTL_MOD, fd, &ev);
	}

}

static void
handle_output(int worker_no, struct io_data_t *client_io_ptr)
{
	int fd = client_io_ptr->fd;
	struct epoll_event ev;
	buffer_t *q = client_io_ptr->out_buf_head;
	int ct;
	bp_t *mybp = g_bufpoll[client_io_ptr->worker_no];
	int can_input = 0;
	if (q) {
		ct = send(fd, q->data, q->size, MSG_NOSIGNAL);
		memcpy(q->data, q->data + ct, q->size - ct);
		q->size -= ct;
		if (q->size == 0) {
			if (q->next >= 0) {
				client_io_ptr->out_buf_head = mybp->base + q->next;
			} else {
				client_io_ptr->out_buf_head = NULL;
				client_io_ptr->out_buf_tail = NULL;
				can_input = 1;
			}
			free_buf(mybp, q);
		}
	}
	if (can_input) {
		ev.events = EPOLLIN;
		ev.data.ptr = client_io_ptr;
		epoll_ctl(g_ep_fd[worker_no], EPOLL_CTL_MOD, fd, &ev);
	}

}


static void *
handle_core(void *param)
{
	register int i;
	int cfd, nfds, case_no;
	struct epoll_event events[MAX_EPOLL_FD];

	struct io_data_t *client_io_ptr;

	struct thread_data_t my_tdata = *(struct thread_data_t *)param;

	int my_epfd = g_ep_fd[my_tdata.worker_no];
	char s_body_buf[MAX_BODY_SIZE] = { 0 };
	char s_head_buf[MAX_BODY_SIZE] = { 0 };

	while (1) {
		nfds = epoll_wait(my_epfd, events, MAX_EPOLL_FD, 1000);
		//printf("nfds:%d\n",nfds);
		if (nfds <= 0 && 0 != g_shutdown_flag) {
			break;
		}

		for (i = 0; i < nfds; i++) {
			client_io_ptr = (struct io_data_t *)events[i].data.ptr;

			if (events[i].events & EPOLLIN) {
				handle_input(my_tdata.worker_no, client_io_ptr, s_body_buf, s_head_buf);

			} else if (events[i].events & EPOLLOUT) {
				handle_output(my_tdata.worker_no, client_io_ptr);

			} else {
				cfd = client_io_ptr->fd;
				case_no = 3;
				destroy_fd(my_epfd, cfd, client_io_ptr, case_no);
			}
		}
	}
	return NULL;
}
