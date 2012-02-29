#include <stdio.h>
#include <stdlib.h>
#include "tst.h"

void output(tst_db* db){

	printf("foo:%llu\n",tst_get(db,"foo"));
	printf("foobar:%llu\n",tst_get(db,"foobar"));
	printf("ac:%llu\n",tst_get(db,"ac"));
	printf("zzz:%llu\n",tst_get(db,"zzz"));
	printf("foonk:%llu\n",tst_get(db,"foonk"));
	printf("haha:%llu\n",tst_get(db,"haha"));

}

void insert_batch(tst_db* db, int total){
	int i;
	char key[256]={0};
	for(i=0;i<total;i++){
		sprintf(key,"%d",i);		
		tst_put(db,key,i);
	}
}

void my_tst_prefix(tst_db* db,const char* key,char * result,int* result_size,int limit)
{
	tst_prefix(db,key,result,result_size,limit,ASC);
}	

void testcase_1(){
	tst_db * db = create_tst_db();
	int i;
	char key[256]={0};

	tst_put(db,"foo",1);
	tst_put(db,"foobar",2);
	tst_put(db,"ac",3);
	tst_put(db,"zzz",4);
	tst_put(db,"foonk",5);
	
	output(db);	

	tst_delete(db,"foobar");	
	printf("======================\n");
	output(db);

	tst_delete(db,"foo");	
	printf("======================\n");
	output(db);

	tst_delete(db,"foonk");	
	printf("======================\n");
	output(db);

	tst_delete(db,"ac");	
	printf("======================\n");
	output(db);

	tst_delete(db,"zzz");	
	printf("======================\n");
	output(db);

	tst_put(db,"foobar",6);
	tst_put(db,"haha",7);
	printf("======================\n");
	output(db);

	tst_delete(db,"haha");
	printf("======================\n");
	output(db);

	insert_batch(db,10000000);
	for(i=10;i<10000000;i++){
		sprintf(key,"%d",i);		
		tst_delete(db,key);
	}
	tst_delete(db,"5");
	for(i=0;i<10;i++){
		sprintf(key,"%d",i);		
		printf("key: %llu\n",tst_get(db,key));
	}
	for(i=0;i<10;i++){
		sprintf(key,"%d",i);		
		tst_delete(db,key);
	}

	tst_delete(db,"foo");
	tst_delete(db,"foobar");
	tst_delete(db,"ac");
	tst_delete(db,"zzz");
	tst_delete(db,"foonk");

	printf("root: %u\n", db->root);

}

void testcase_2()
{
	char result[250000];
	int result_size=0;
	tst_db * db = create_tst_db();

	tst_put(db,"foo",1);
	tst_put(db,"foobar",2);
	tst_put(db,"ac",3);
	tst_put(db,"acf",12);
	tst_put(db,"a1243",666);
	tst_put(db,"fork",777);
	tst_put(db,"zzz",4);
	tst_put(db,"foonk",5);
	
	printf("starts with fo\n");
	my_tst_prefix(db, "fo",result, &result_size,10);		
	printf("%s\n", result);

	printf("starts with zz\n");
	my_tst_prefix(db,"zz",result,&result_size,10);	
	printf("%s\n", result);

	printf("starts with foo\n");
	my_tst_prefix(db,"foo",result,&result_size,10);	
	printf("%s\n", result);


	printf("starts with a\n");
	my_tst_prefix(db,"a",result,&result_size,10);	
	printf("%s\n", result);
}
void testcase_3()
{
	tst_db * db = create_tst_db();
	char result[250000];
	int result_size;

	insert_batch(db,100000);	
	printf("starts with 126\n");
	my_tst_prefix(db,"126",result,&result_size,20);	
	printf("%s\n", result);

}

void testcase_4()
{
	tst_db * db = create_tst_db();
	char result[250000];
	int result_size;

	insert_batch(db,100000);	
	//tst_put(db,"ab",1);
	//tst_put(db,"za",2);
	//tst_put(db,"z",3);
	printf("less than 126\n");
	tst_less(db,"126",result,&result_size,30);	
	printf("%s\n", result);

}
void testcase_5()
{
	tst_db * db = create_tst_db();
	char result[250000];
	int result_size;

	insert_batch(db,100000);	
	//tst_put(db,"ab",1);
	//tst_put(db,"za",2);
	//tst_put(db,"z",3);
	printf("greater than 126\n");
	tst_greater(db,"126",result,&result_size,30);	
	printf("%s\n", result);

}

int main(int argc ,char * argv[]){
	//testcase_1();	
	testcase_2();

	testcase_3();
	testcase_4();
	testcase_5();
	return 0;
}
