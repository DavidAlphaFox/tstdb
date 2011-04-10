#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"
int main(){
	tst_db* db = new_tst_db(10);
	put(db,"123",123);
	printf("size: %d\n",db->size);
	put(db,"-sdf",456);
	printf("size: %d\n",db->size);
	put(db,"xxx",789);
	printf("size: %d\n",db->size);
	put(db,"xxx",111);
	printf("size: %d\n",db->size);
	put(db,"-sdf",222);
	printf("size: %d\n",db->size);
	put(db,"123",333);
	printf("size: %d\n",db->size);
	printf("%ld,%ld,%ld\n",get(db,"xxx"),get(db,"-sdf"),get(db,"123"));
	printf("size: %d\n",db->size);
	printf("get a null string\n");
	printf("%ld\n",get(db,""));
	printf("put a null string\n");
	put(db,"",213);
	return 0;
}
