#ifndef BUFFERPOLL
#define BUFFERPOLL

#define BUF_UNIT_SIZE 512
struct buffer_t{
	char data[BUF_UNIT_SIZE];
	int size;
	int next;
};
typedef struct buffer_t buffer_t;

struct buffer_pool{
	buffer_t * base;
	int head;
};

typedef struct buffer_pool bp_t;

bp_t* bp_new(unsigned int n_items);

buffer_t* alloc_buf(bp_t *this);

buffer_t* get_buf(bp_t *this,unsigned int nth);

void free_buf(bp_t *this , buffer_t* buf);

#endif
