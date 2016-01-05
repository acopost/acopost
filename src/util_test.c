#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	return test_reverse();
}
