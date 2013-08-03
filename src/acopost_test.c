/*
  Copyright (c) 2013, ACOPOST Developers Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the ACOPOST Developers Team nor the names of
     its contributors may be used to endorse or promote products
     derived from this software without specific prior written
     permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*/

#include "config-common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
/* #include <sys/types.h> */
#include <time.h>

#include "array.h"
#include "hash.h"
#include "mem.h"
#include "primes.h"
#include "util.h"

int mem_test ()
{
    void *p;

    /* Allocates 10Mb of memory, reallocates it to 1Mb,
     * reallocates it to 25Mb and then frees it */
    printf("Testing 'mem_malloc()'...\n");
    p = mem_malloc(1048576 * 10);

    printf("Testing 'mem_realloc()' with smaller size...\n");
    p = mem_realloc(p, 1048576);

    printf("Testing 'mem_realloc()' with bigger size...\n");
    p = mem_realloc(p, 1048576 * 25);

    printf("Testing 'mem_free()'...\n");
    mem_free(p);

    return 0;
}

int array_test ()
{
    array_pt arr1, arr2, arr3, arr4;
    int *i, *a, *b, *c, *d, *e;
    int j;

    /* memory init and dummy values */
    i = mem_malloc(sizeof(int));
    *i = 13051983;
    a = mem_malloc(sizeof(int)); *a = 1;
    b = mem_malloc(sizeof(int)); *b = 2;
    c = mem_malloc(sizeof(int)); *c = 3;
    d = mem_malloc(sizeof(int)); *d = 4;
    e = mem_malloc(sizeof(int)); *e = 1;

    printf("Testing 'array_new()'...\n");
    arr1 = array_new(1000);

    printf("Testing 'array_fill()'...\n");
    array_fill(arr1, i);

    printf("Testing 'array_new_fill()'...\n");
    arr2 = array_new_fill(1000, i);

    printf("Testing 'array_clear()'...\n");
    array_clear(arr1);

    printf("Testing 'array_add()'...\n");
    for (j = 0; j < 100; j++)
        array_add(arr1, i);
    for (j = 0; j < 10000; j++)
        array_add(arr1, i);
//    printf("--> count: %i size: %i\n", arr1->count, arr1->size);
//    printf("--> v[0]: %i\n", *(int *)arr1->v[0]);

    arr4 = array_new(0);
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, a));
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, b));
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, c));
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, d));
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, e));
    printf("Testing 'array_add_unique()'... ret=%zu\n", array_add_unique(arr4, a));
    printf("--> count: %zu\n", arr4->count);

    printf("Testing 'array_add'...\n");
    array_add(arr4, a);
    array_add(arr4, b);
    array_add(arr4, c);
    printf("--> count: %zu\n", arr4->count);

    printf("Testing 'array_delete_duplicates()'...\n");
    array_delete_duplicates(arr4);
    printf("--> count: %zu\n", arr4->count);

    printf("Testing 'array_delete_item()'...\n");
    array_delete_item(arr4, a);
    printf("--> count: %zu\n", arr4->count);

    /* TODO: the test should be more stressing, many indexes, duplicates, start, end, etc. */
    printf("Testing 'array_delete_index()'...\n");
    array_delete_index(arr4, 1);
    printf("--> count: %zu\n", arr4->count);

    /* TODO: valgrind is accusing a memory leak in array_clone */
    printf("Testing 'array_clone()'...\n");
    arr3 = array_clone(arr1);
    printf("--> arr1->size: %zu, arr1->v address: %p\n", arr1->size, arr1->v);
    printf("--> arr3->size: %zu, arr3->v address: %p\n", arr3->size, arr3->v);
    printf("--> arr1->v[0]: %i, arr1->v[0] address: %p\n", *(int *)arr1->v[0], arr1->v[0]);
    printf("--> arr3->v[0]: %i, arr3->v[0] address: %p\n", *(int *)arr3->v[0], arr3->v[0]);
    printf("--> &i: %p\n", i);

    printf("Testing 'array_trim()'...\n");
    printf("--> arr1->size: from %zu", arr1->size);
    array_trim(arr1);
    printf(" to %zu\n", arr1->size);
    printf("--> arr4->size: from %zu", arr4->size);
    array_trim(arr4);
    printf(" to %zu\n", arr4->size);

    printf("Testing 'array_get()'...\n");
    array_get(arr4, 0);

    printf("Testing 'array_free()'...\n");
    array_free(arr1);
    array_free(arr2);
    array_free(arr3);
    array_free(arr4);

    /* clean up */
    mem_free(i);
    mem_free(a);
    mem_free(b);
    mem_free(c);
    mem_free(d);
    mem_free(e);

    return 0;
}


int primes_test ()
{
#define PRIME_TEST (10)
#define NO_OF_KNOWN_PRIMES (8)
    int i, n, n_next;
    int known_primes[NO_OF_KNOWN_PRIMES] = {
	3,
	5,
	7,
	11,
	13,
	17,
	19,
	23
    };

    srand48(time(NULL));
  
    
    printf("Testing 'primes_rabin()'...\n");

    for (i = 0;
	 i < NO_OF_KNOWN_PRIMES;
	 ++i) {
	n = known_primes[i];
	printf("Now testing wither %d is prime...\n", n);
	if (!miller_rabin_size_t((size_t) n)) {
	    printf("Error: %d is prime", n);
	    return 1;
	}
    }

    printf("Testing 'primes_next()'...\n");
    for (i = 0;
	 i < NO_OF_KNOWN_PRIMES-1;
	 ++i) {
	n = known_primes[i];
	n_next = known_primes[i+1];
	printf("Now finding next prime after %d is prime...\n", n);

	unsigned long next_prime = miller_rabin_next_prime_size_t(n+1);
	if (next_prime != (unsigned long) n_next) {
	    printf("Error: The next prime after %d is not %d but %d\n", n, (int) next_prime, n_next);
	    return 1;
	}
    }

    return 0;
}


int main (int argc, char *argv[])
{
    int bContinue;

    bContinue = !mem_test();
    bContinue = bContinue ? !array_test() : 0;
    bContinue = bContinue ? !primes_test() : 0;

    /* Free the memory held by util.c. */
    util_teardown();

    if (bContinue) {
	printf("SUCCESS: All tests succeeded.\n");
	return 0;
    } else {
	printf("ERROR: At least one test failed.\n");
	return 1;
    }
}

