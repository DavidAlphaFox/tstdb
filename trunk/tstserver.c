#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "tst.h"

#define MAX_EPOLL_FD 4096
#define IN_BUF_SIZE 1024 
#define MAX_VALUE_SIZE (1<<20)
#define MAX_FD_OPEN 32767

int ep_fd;
tst_db *g_db;
char big_buffer[MAX_VALUE_SIZE];// the longest value in memcached is 1MB

typedef struct payload_t{
	char * read_buf;
	char * write_buf;
	char * putkey;
	int need_read;
	int need_write;
	int write_cur;
	int read_cur;
}payload_t;

payload_t dataTable[MAX_FD_OPEN];

static void *handle_func(void *);
static void setnonblocking(int fd)
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

static void exit_hook(int number)
{
	close(ep_fd);
	tst_close(g_db);
	printf(">> Bye.\n");
	exit(0);
}
int main(int argc, char **argv)
{
	char *ip_binding = "0.0.0.0";
	int port_listening = 8402;
	char *db_path = "/tmp/tst.db";
	int opt;
	int on = 1;

	int listen_fd, client_fd;

	struct sockaddr_in server_addr, client_addr;
	socklen_t client_n;
	struct epoll_event ev;


	pthread_t tid;
	pthread_attr_t attr;

	while ((opt = getopt(argc, argv, "l:p:f:h")) != -1) {
		switch (opt) {
		case 'l':
			ip_binding = strdup(optarg);
			break;
		case 'p':
			port_listening = atoi(optarg);
			break;
		case 'f':
			db_path = strdup(optarg);
			break;
		case 'h':
			printf("tstserver help.\r\n"
			"\t-l <binding ip> , default value is 0.0.0.0\r\n"
			"\t-p <listening port>, default value is 8402\r\n"
			"\t-f <db file path>, default value is /tmp/tst.db\r\n"
			"\t-h show help\r\n"
			"\r\n");
			exit(0);
		}

	}
	printf(">> IP listening:%s\n", ip_binding);
	printf(">> port: %d\n", port_listening);
	printf(">> db path: %s\n", db_path);

	g_db = tst_open(db_path);
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listen_fd) {
		perror("listen faild!");
		exit(-1);
	}
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((short) port_listening);
	server_addr.sin_addr.s_addr = inet_addr(ip_binding);

	if (-1 ==
	    bind(listen_fd, (struct sockaddr *) &server_addr,
		 sizeof(server_addr))) {
		perror("bind error");
		exit(-1);
	}

	if (-1 == listen(listen_fd, 32)) {
		perror("listen error");
		exit(-1);
	}


	bzero(dataTable,sizeof(dataTable));
	ep_fd = epoll_create(MAX_EPOLL_FD);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, handle_func, NULL) != 0) {
		fprintf(stderr, "pthread_create failed\n");
		return -1;
	}

	signal (SIGPIPE, SIG_IGN);
	signal (SIGINT, exit_hook);
	signal (SIGKILL, exit_hook);
	signal (SIGQUIT, exit_hook);
	signal (SIGTERM, exit_hook);
	signal (SIGHUP, exit_hook);


	while ((client_fd =
		accept(listen_fd, (struct sockaddr *) &client_addr,
		       &client_n)) > 0) {
		setnonblocking(client_fd);
		ev.data.fd = client_fd;
		ev.events = EPOLLIN;
		epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &ev);
		//printf("connection from %s\n",inet_ntoa(client_addr.sin_addr));

	}
	return 0;
}


static void *handle_func(void *p)
{
	int i, ret, cfd, nfds;
	struct epoll_event ev, events[MAX_EPOLL_FD];
	char buffer[IN_BUF_SIZE];
	int flag,expire,total,read_count,header_len,read_limit;
	unsigned int len_of_value;
	char key[256]={0};	
	char rsps_msg[256] ={0};
	time_t now;

	while (1) {
		nfds = epoll_wait(ep_fd, events, MAX_EPOLL_FD, -1);

		for (i = 0; i < nfds; i++) {
			if(events[i].data.fd>=MAX_FD_OPEN){
				cfd = events[i].data.fd;
				ev.data.fd = cfd;
				epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
				close(cfd);
				continue;	
			}
			//printf("%d,%d\n",i,events[i].data.fd);
			bzero(buffer,sizeof(buffer));
			if (events[i].events & EPOLLIN) {
				cfd = events[i].data.fd;
				ret = 0;
				if(dataTable[cfd].need_read==0){
					ret = recv(cfd, buffer, sizeof(buffer), 0);
					//printf("ret : %d\n",ret);
				}
				else{
					//printf("need read %d\n",dataTable[cfd].need_read);
					//printf("read_cur %d\n",dataTable[cfd].read_cur);
					read_limit = dataTable[cfd].need_read-dataTable[cfd].read_cur;
					read_limit = read_limit>sizeof(buffer)?sizeof(buffer):read_limit;
					ret = recv(cfd, buffer, read_limit,0);
					//printf("cfd %d\n",cfd);
					//printf("%d > %d\n",dataTable[cfd].need_read,dataTable[cfd].read_cur);
				}
				if(ret<=0){
					ev.data.fd = cfd;
				       	epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
					close(cfd);
					continue;
				}
				read_count = ret;
				if(dataTable[cfd].need_read ==0 &&  sscanf(buffer,"get %s\r\n",key) == 1 ){
					//printf("client want to get %s\n",key);	
					tst_get(g_db,key,big_buffer,&len_of_value);		
					//printf("v len:%d\n",len_of_value);
					if(len_of_value==0){//not exits
						dataTable[cfd].write_buf = (char*)malloc(5);
						memcpy(dataTable[cfd].write_buf,"END\r\n",5);
						dataTable[cfd].write_cur = 0;
						dataTable[cfd].need_write = 5;
						ev.data.fd = cfd;
						ev.events = EPOLLOUT;
						epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);	
						continue;
					}
					
					memcpy(&flag,big_buffer,4);
					memcpy(&expire,big_buffer+4,4);

					if(expire!=0){// has expire flag
						time(&now);
						if(expire <= now){//expired
							tst_put(g_db,key,"",0);		
							dataTable[cfd].write_buf = (char*)malloc(5);
							memcpy(dataTable[cfd].write_buf,"END\r\n",5);
							dataTable[cfd].write_cur = 0;
							dataTable[cfd].need_write = 5;
							ev.data.fd = cfd;
							ev.events = EPOLLOUT;
							epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);	
							continue;
						}
					}
					len_of_value -= 8; //8 is the length of flag and expire;
					if(len_of_value<0){
						printf("data corrupted");
						exit(1);
					}
					sprintf(rsps_msg,"VALUE %s %d %d\r\n",key,flag,len_of_value);					
					header_len = strlen(rsps_msg);
					dataTable[cfd].write_buf = (char*)malloc(len_of_value+header_len+7);
					memcpy(dataTable[cfd].write_buf,rsps_msg,header_len);
					dataTable[cfd].write_cur = 0;
					dataTable[cfd].need_write = len_of_value+header_len+7;
					memcpy(dataTable[cfd].write_buf+header_len,big_buffer+8,len_of_value);
					memcpy(dataTable[cfd].write_buf+header_len+len_of_value,"\r\nEND\r\n",7);//7 is the length of "\r\nEND\r\n"
					ev.data.fd = cfd;
					ev.events = EPOLLOUT;
					epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);		
				}
				else if(dataTable[cfd].need_read ==0 && sscanf(buffer,"set %s %d %d %d\r\n",key,&flag,&expire,&total)==4){
					//printf("client want to put %s\n",key);
					//printf("flag: %d\n",flag);
					//printf("expire: %d\n",expire);
					//printf("total: %d\n",total);
					if(total<0 || total>MAX_VALUE_SIZE){
						dataTable[cfd].write_buf = (char*)malloc(7);
						memcpy(dataTable[cfd].write_buf,"ERROR\r\n",7);
						dataTable[cfd].need_write = 7;
						dataTable[cfd].write_cur = 0;
						ev.data.fd = cfd;
						ev.events = EPOLLOUT;
						epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);			
						continue;
					}
					header_len = strstr(buffer,"\r\n")-buffer+2; 
					//printf("header_len:%d\n",header_len);
					//printf("read_count:%d\n",read_count);
					if(expire!=0){
						time(&now);
						expire += now;
					}
					memcpy(big_buffer,(char*)&flag,4);
					memcpy(big_buffer+4,(char*)&expire,4);
					memmove(big_buffer+8,buffer+header_len,read_count-header_len);
					total +=8 ; //flag(4byte),expire(4byte)
					if(header_len+total-8<read_count){//one round enough
						tst_put(g_db,key,big_buffer,total);
						dataTable[cfd].write_buf = (char*)malloc(8);
						memcpy(dataTable[cfd].write_buf,"STORED\r\n",8);
						dataTable[cfd].need_write = 8;
						dataTable[cfd].write_cur = 0;
						ev.data.fd = cfd;
						ev.events = EPOLLOUT;
						epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);				
					}
					else{
						dataTable[cfd].read_buf = (char*)malloc(total+2);
                                                memcpy(dataTable[cfd].read_buf,big_buffer,read_count-header_len+8);
                                                dataTable[cfd].read_cur = read_count-header_len+8;
                                                dataTable[cfd].putkey = strdup(key);
                                                dataTable[cfd].need_read = total+2;	
					}

				}
				else if(dataTable[cfd].need_read ==0 && sscanf(buffer,"delete %s",key)==1){
					tst_get(g_db,key,big_buffer,&len_of_value);
					if(len_of_value==0){//not found
						sprintf(rsps_msg,"NOT_FOUND\r\n");
					}else{
						sprintf(rsps_msg,"DELETED\r\n");
						tst_put(g_db,key,"",0);
					}	
					dataTable[cfd].write_buf = (char*)malloc(strlen(rsps_msg));
					memcpy(dataTable[cfd].write_buf,rsps_msg,strlen(rsps_msg));
					dataTable[cfd].write_cur = 0;
					dataTable[cfd].need_write = strlen(rsps_msg);
					ev.data.fd = cfd;
					ev.events = EPOLLOUT;
					epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);
				}
				else if(dataTable[cfd].need_read!=0){
					memcpy(dataTable[cfd].read_buf+dataTable[cfd].read_cur,buffer,read_count);
					dataTable[cfd].read_cur += read_count;
					//printf("%d-%d\n",dataTable[cfd].read_cur,dataTable[cfd].need_read);
					if(dataTable[cfd].need_read == dataTable[cfd].read_cur){
						//printf("read over %d\n",dataTable[cfd].read_cur);
						tst_put(g_db,dataTable[cfd].putkey,dataTable[cfd].read_buf,dataTable[cfd].read_cur-2);// strlen(\r\n)==2
						free(dataTable[cfd].read_buf);
						free(dataTable[cfd].putkey);
						dataTable[cfd].read_buf =0;
						dataTable[cfd].putkey = 0;
						dataTable[cfd].need_read = 0;
						dataTable[cfd].read_cur = 0;
						
						dataTable[cfd].write_buf = (char*)malloc(8);
						memcpy(dataTable[cfd].write_buf,"STORED\r\n",8);
						dataTable[cfd].need_write = 8;
						dataTable[cfd].write_cur = 0;
						ev.data.fd = cfd;
						ev.events = EPOLLOUT;
						epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);
					}
				}
				else{
					//printf("UnExpected String!,%s\n",buffer);
					//printf("ReadCount: %d\n",read_count);
					/*dataTable[cfd].write_buf = (char*)malloc(7);
					memcpy(dataTable[cfd].write_buf,"ERROR\r\n",7);
					dataTable[cfd].need_write = 7;
					dataTable[cfd].write_cur = 0;
					ev.data.fd = cfd;
					ev.events = EPOLLOUT;
					epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);*/
				}
				
			} else if (events[i].events & EPOLLOUT) {
				cfd = events[i].data.fd;
				ret =
				    send(cfd, dataTable[cfd].write_buf+dataTable[cfd].write_cur, 
					      dataTable[cfd].need_write-dataTable[cfd].write_cur,
					 MSG_NOSIGNAL);
				//printf("sent %d\n",ret);
				if(ret<0){
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						continue;
					}else{
						free(dataTable[cfd].write_buf);
						dataTable[cfd].write_buf = 0;
						dataTable[cfd].write_cur = 0;
						dataTable[cfd].need_write = 0;
						ev.data.fd = cfd;
						epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
						close(cfd);
						continue;
					}
				}
				dataTable[cfd].write_cur += ret;
				if(dataTable[cfd].write_cur == dataTable[cfd].need_write){
					free(dataTable[cfd].write_buf);
					dataTable[cfd].write_buf = 0;
					dataTable[cfd].write_cur = 0;
					dataTable[cfd].need_write = 0;
					ev.data.fd = cfd;
					ev.events = EPOLLIN;
					epoll_ctl(ep_fd,EPOLL_CTL_MOD,cfd,&ev);	
				}	
			}
		}
	}
	return NULL;
}
