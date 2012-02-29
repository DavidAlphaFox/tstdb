#include "buffer_pool.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

bp_t *
bp_new(unsigned int n_items)
{
	bp_t *this;
	buffer_t *base;
	int i;
	this = malloc(sizeof(bp_t));
	assert(this > 0);
	assert(n_items > 0);
	base = (buffer_t *) calloc(n_items, sizeof(buffer_t));
	assert(base > 0);
	this->base = base;
	this->head = 0;
	for (i = 0; i < n_items; i++) {
		this->base[i].size = 0;
		this->base[i].next = i + 1;
	}
	this->base[n_items - 1].next = -1;	//last element 
	return this;
}

buffer_t *
alloc_buf(bp_t * this)
{
	buffer_t *buf;
	int next_head;
	assert(this->head >= 0);
	next_head = this->base[this->head].next;
	buf = this->base + this->head;
	buf->next = -1;
	buf->size = 0;
	this->head = next_head;
	//printf("pop head:%d\n",this->head);

	assert(this->head >= 0);
	return buf;
}

buffer_t *
get_buf(bp_t * this, unsigned int nth)
{
	assert(this->head >= 0 && nth >= 0);
	return this->base + nth;
}

void
free_buf(bp_t * this, buffer_t * buf)
{
	int cur_head;
	//printf("%lu\n",(long)buf);
	cur_head = this->head;
	assert(cur_head >= 0);
	this->head = buf - this->base;
	assert(this->head >= 0);
	buf->size = 0;
	buf->next = cur_head;
	//printf("push head:%d\n",this->head);
}
