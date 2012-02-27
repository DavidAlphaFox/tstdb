#ifndef CMD_PROCESS
#define CMD_PROCESS

void cmd_do_get(struct io_data_t* p, const char* header ,int r_sign); //r_sign: gets or get

void cmd_do_prefix(struct io_data_t* p, const char* header); 

void cmd_do_set(struct io_data_t* p, const char* header, const char* body );

void cmd_do_delete(struct io_data_t* p, const char* header );

void cmd_do_cas(struct io_data_t *p , const char* header, const char* body);

void cmd_do_incr(struct io_data_t* p, const char* header);

void cmd_do_decr(struct io_data_t* p, const char* header);

#endif
