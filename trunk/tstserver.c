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
#define IN_BUF_SIZE 500

int ep_fd;
tst_db *g_db;
char big_buffer[1<<20];// the longest value in memcached is 1MB

typedef struct{
	char * buf;
	char * putkey;
	int need_read;
	int need_write;
	int cur;
}payload_t;

payload_t dataTable[32767];

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
	int flag,expire,total,read_count,header_len;
	unsigned int len_of_value;
	char key[256]={0};	
	char rsps_msg[256] =
	    "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\nContent-Type: text/html\r\n\r\nHello";

	while (1) {
		nfds = epoll_wait(ep_fd, events, MAX_EPOLL_FD, -1);

		for (i = 0; i < nfds; i++) {
			bzero(buffer,sizeof(buffer));
			if (events[i].events & EPOLLIN) {
				cfd = events[i].data.fd;
				ret = 0;
				ret = recv(cfd, buffer, sizeof(buffer), 0);
				if(ret<0){
					ev.data.fd = cfd;
				       	epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
					close(cfd);
					continue;
				}
				read_count = ret;
				if(dataTable[cfd].buf!=NULL && dataTable[cfd].need_read>0){
					memcpy(dataTable[cfd].buf+dataTable[cfd].cur,buffer,read_count);
					dataTable[cfd].cur += read_count;
					//printf("%d,%d\n",dataTable[cfd].cur,read_count);
					if(dataTable[cfd].cur>=dataTable[cfd].need_read){
						//printf("kkkkk\n");
						tst_put(g_db,dataTable[cfd].putkey,dataTable[cfd].buf,dataTable[cfd].need_read);
						ret = send(cfd,"STORED\r\n",8,MSG_NOSIGNAL);		

						free(dataTable[cfd].buf);
						free(dataTable[cfd].putkey);
						bzero(&dataTable[cfd],sizeof(payload_t));

						if(ret<0){
					        	ev.data.fd = cfd;
				        		epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
							close(cfd);
						}	
					}		
				}
				else if( sscanf(buffer,"get %s",key) == 1 ){
					//printf("client want to get %s\n",key);	
					tst_get(g_db,key,big_buffer,&len_of_value);		
					//printf("v len:%d\n",len_of_value);
					if(len_of_value==0){
						ret = send(cfd,"END\r\n",5,MSG_NOSIGNAL);
						if(ret<0){
                                                 	ev.data.fd = cfd;
                                                 	epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
                                                 	close(cfd);
                                         	}
						continue;
					}
					sprintf(rsps_msg,"VALUE %s 0 %d\r\n",key,len_of_value);					
					header_len = strlen(rsps_msg);
					dataTable[cfd].buf = (char*)malloc(len_of_value+header_len+7);
					memcpy(dataTable[cfd].buf,rsps_msg,header_len);
					dataTable[cfd].cur = 0;
					dataTable[cfd].need_write = len_of_value+header_len+7;
					memcpy(dataTable[cfd].buf+header_len,big_buffer,len_of_value);
					memcpy(dataTable[cfd].buf+header_len+len_of_value,"\r\nEND\r\n",7);
					ev.data.fd = cfd;
					ev.events = EPOLLOUT;
					epoll_ctl(ep_fd, EPOLL_CTL_MOD, cfd, &ev);		
				}
				else if(sscanf(buffer,"set %s %d %d %d\r\n",key,&flag,&expire,&total)==4){
					//printf("client want to put %s\n",key);
					//printf("flag: %d\n",flag);
					//printf("expire: %d\n",expire);
					//printf("total: %d\n",total);
					header_len = strstr(buffer,"\r\n")-buffer+2; 
					//printf("header_len:%d\n",header_len);
					//printf("read_count:%d\n",read_count);
					if(header_len+total<read_count){//one round enough
						tst_put(g_db,key,buffer+header_len,total);
						ret = send(cfd,"STORED\r\n",8,MSG_NOSIGNAL);		
						if(ret<0){
					        	ev.data.fd = cfd;
				        		epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
							close(cfd);
						}
					}

					else{
						//printf("need more rounds to receiv\n");
						dataTable[cfd].buf = (char*)malloc(total+2);
						memcpy(dataTable[cfd].buf,buffer+header_len,read_count-header_len);
						dataTable[cfd].cur = read_count-header_len;
						dataTable[cfd].putkey = strdup(key);
						dataTable[cfd].need_read = total;
					}
				}
				else{
					printf("%s\n",buffer);
					ret = send(cfd,"ERROR\r\n",7,MSG_NOSIGNAL);
					if(ret<0){
					        ev.data.fd = cfd;
				        	epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
						close(cfd);
					}
				}
				
			} else if (events[i].events & EPOLLOUT) {
				cfd = events[i].data.fd;
				ret =
				    send(cfd, dataTable[cfd].buf+dataTable[cfd].cur, 
					      dataTable[cfd].need_write-dataTable[cfd].cur,
					 MSG_NOSIGNAL);
				//printf("sent %d\n",ret);
				if(ret<0){
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						continue;
					}else{
						free(dataTable[cfd].buf);
						bzero(&dataTable[cfd],sizeof(payload_t));
						ev.data.fd = cfd;
						epoll_ctl(ep_fd,EPOLL_CTL_DEL,cfd,&ev);
						close(cfd);
						continue;
					}
				}
				dataTable[cfd].cur += ret;
				if(dataTable[cfd].cur == dataTable[cfd].need_write){
					free(dataTable[cfd].buf);
					bzero(&dataTable[cfd],sizeof(payload_t));
					ev.data.fd = cfd;
					ev.events = EPOLLIN;
					epoll_ctl(ep_fd,EPOLL_CTL_MOD,cfd,&ev);	
				}	
			}
		}
	}
	return NULL;
}
