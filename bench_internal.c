#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"
int main(){
	tst_db* db = create_tst_db();
	int i;
	char buf[256]={0};
	char mem[100][256];
	printf("\n=============\n");
	for(i=0;i<10000000;i++){
		sprintf(buf,"%d",(int)((double)rand()/RAND_MAX*100000000));
		//printf("%s\n",buf);
		put(db,buf,(uint64)i+1);
		if(i%20000==0){
			printf("%d\n",i);
		}
		if(i<100){
			strcpy(mem[i],buf);
		}
	}
	printf("db size after inserting %d\n",db->size);
	for(i=0;i<100;i++){
		printf("result: %ld\n",get(db,mem[i]));
	}
	printf("\n~~~~~~~~\n");
	return 0;
}
