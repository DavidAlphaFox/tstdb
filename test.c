#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"
char bigvalue[10000];

int main(int argc,char* argv[])
{
	tst_db *db = tst_open("test.db");
	char value[25600] = { 0 };
	unsigned int len_of_value = 0;
	if(argc<2){
		printf("need arg : r or w\n");
		exit(0);
	}
	if(argv[1][0]=='w'){
		tst_put(db, "123", "123", 3);
		tst_put(db, "-sdf", "456", 3);
		tst_put(db, "xxx", "789", 3);

		tst_put(db, "xxx", "1234567890abc", strlen("1234567890abc"));
		tst_put(db, "-sdf", "222", 3);
		tst_put(db, "123", "3337", 4);
		memset(bigvalue,'Q',sizeof(bigvalue));
		tst_put(db,"xyz",bigvalue,sizeof(bigvalue));
	}
	if(argv[1][0]=='r'){
		tst_get(db, "123", value, &len_of_value);
		printf("%s \t %d\n", value, len_of_value);
		tst_get(db, "-sdf", value, &len_of_value);
		printf("%s \t %d\n", value, len_of_value);
		tst_get(db, "xxx", value, &len_of_value);
		printf("%s \t %d\n", value, len_of_value);
		tst_get(db, "xyz", value, &len_of_value);
		printf("%s \t %d\n", value, len_of_value);

	}
	tst_close(db);
	return 0;
}
