#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* OK_STR = "ok";
static const char* NOT_OK_STR = "not ok";

int test_substr() {

	char* string0 = "";
	char* string1 = "12345";
	char* string2 = "234";
	char* string3 = "34";
	char* string4 = "1234";
	char* string5 = "45";


	char* tmp = NULL;
        size_t tmp_n = 0;
	char* tmp2 = NULL;

	const char* test_res_str = NOT_OK_STR;

	/* On NULL should return NULL */
	test_res_str = NOT_OK_STR;
	tmp2 = reverse(NULL, &tmp, &tmp_n);
	if(tmp2 == NULL) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);

	/* On empty should return empty */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string0, 1, 1, &tmp, &tmp_n);
	if(!strcmp(string0,tmp2)) {
		test_res_str = OK_STR;
	}

	/* On string1, 0, strlen(string1) should return string1 */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string1, 0, strlen(string1), &tmp, &tmp_n);
	if(!strcmp(string1,tmp2)) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);

	/* On string1, 1, 3 should return string2 */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string1, 1, 3, &tmp, &tmp_n);
	if(!strcmp(string2,tmp2)) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);

	/* On string1, 3, -2 should return string3 */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string1, 3, -2, &tmp, &tmp_n);
	if(!strcmp(string3,tmp2)) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);

	/* On string1, 3, -6 should return string4 */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string1, 3, -6, &tmp, &tmp_n);
	if(!strcmp(string4,tmp2)) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);

	/* On string1, 3, 10 should return string5 */
	test_res_str = NOT_OK_STR;
	tmp2 = substr(string1, 3, 10, &tmp, &tmp_n);
	if(!strcmp(string5,tmp2)) {
		test_res_str = OK_STR;
	}
	printf("%s\n", test_res_str);


	if(tmp != NULL) {
		free(tmp);
		tmp = NULL;
	}
	return 0;
}

int test_reverse() {

	char* string0 = "";
	char* string1 = "abcdefghijklmnopqrstuvwxyz";
	char* string2 = "zyxwvutsrqponmlkjihgfedcba";

	char* tmp = NULL;
        size_t tmp_n = 0;
	char* tmp2 = NULL;

	/* On NULL should return NULL */
	tmp2 = reverse(NULL, &tmp, &tmp_n);
	if(tmp2 == NULL) {
		printf("ok\n");
	}

	/* On empty should return empty */
	tmp2 = reverse(string0, &tmp, &tmp_n);
	if(!strcmp(string0,tmp2)) {
		printf("ok\n");
	}

	/* On string1 should return string2 */
	tmp2 = reverse(string1, &tmp, &tmp_n);
	if(!strcmp(string2,tmp2)) {
		printf("ok\n");
	}

	/* On string2 should return string1 */
	tmp2 = reverse(string2, &tmp, &tmp_n);
	if(!strcmp(string1,tmp2)) {
		printf("ok\n");
	}

	if(tmp != NULL) {
		free(tmp);
		tmp = NULL;
	}
	return 0;
}

int main(int argc, char*argv[]) {
	return test_reverse() || test_substr();
}
