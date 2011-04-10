#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"
int main(int argc, char* argv[]){
	tst_db * db = tst_open("a.db");
	int i;
	unsigned int len_of_v;
	char key[256]={0};
	char value[256]={0};
	if(strcmp(argv[1],"w")==0){
		for(i=0;i<10000000;i++){
			sprintf(key,"%d",rand());
			sprintf(value,"%d",i);	
			tst_put(db,key,value,strlen(value));
			if(i%20000==0){
				printf("%d\n",i);
			}
		}
	}
	if(strcmp(argv[1],"r")==0){
		for(i=0;i<10000000;i++){
			sprintf(key,"%d",rand());
			tst_get(db,key,value,&len_of_v);	
			value[len_of_v]='\0';
			if(len_of_v==0)break;//not found
			if(i%20000==0)
				printf("%s\n",value);
		}
	}
	tst_close(db);
	return 0;
}
