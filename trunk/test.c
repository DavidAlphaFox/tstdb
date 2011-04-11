#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"
int main()
{
	tst_db *db = tst_open("test.db");
	char value[256] = { 0 };
	unsigned int len_of_value = 0;
	tst_put(db, "123", "123", 3);
	tst_put(db, "-sdf", "456", 3);
	tst_put(db, "xxx", "789", 3);

	tst_put(db, "xxx", "1234567890abc", strlen("1234567890abc"));
	tst_put(db, "-sdf", "222", 3);
	tst_put(db, "123", "3337", 4);

	tst_get(db, "123", value, &len_of_value);
	printf("%s \t %d\n", value, len_of_value);
	tst_get(db, "-sdf", value, &len_of_value);
	printf("%s \t %d\n", value, len_of_value);
	tst_get(db, "xxx", value, &len_of_value);
	printf("%s \t %d\n", value, len_of_value);
	len_of_value = 0;
	memset(value, 0, sizeof(value));
	tst_put(db, "xxx", "", 0);
	tst_get(db, "xxx", value, &len_of_value);
	printf("%s \t %d\n", value, len_of_value);
	tst_close(db);
	return 0;
}
