#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
    array_pt arr;

    printf("Testing 'array_new()'...\n");
    arr = array_new(1000);

    printf("Testing 'array_free()'...\n");
    array_free(arr);

    return 0;
}

int main (int argc, char *argv[])
{
     mem_test();
     array_test();

     return 0;
}

