#ifndef TSTSERVER
#define TSTSERVER
struct io_data_t {
	int fd;
	int worker_no;
	int header_len;
	int body_len;
	struct sockaddr_in addr;
	buffer_t *in_buf_head,*in_buf_tail,*out_buf_head,*out_buf_tail;
	int in_buf_len,out_buf_len;	
};


struct thread_data_t {
	int worker_no;
};

#define WORKER_COUNT 2 

#endif

