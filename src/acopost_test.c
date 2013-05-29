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
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, a));
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, b));
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, c));
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, d));
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, e));
    printf("Testing 'array_add_unique()'... ret=%d\n", array_add_unique(arr4, a));
    printf("--> count: %d\n", arr4->count);

    printf("Testing 'array_add'...\n");
    array_add(arr4, a);
    array_add(arr4, b);
    array_add(arr4, c);
    printf("--> count: %d\n", arr4->count);

    printf("Testing 'array_delete_duplicates()'...\n");
    array_delete_duplicates(arr4);
    printf("--> count: %d\n", arr4->count);

    printf("Testing 'array_delete_item()'...\n");
    array_delete_item(arr4, a);
    printf("--> count: %d\n", arr4->count);

    /* TODO: the test should be more stressing, many indexes, duplicates, start, end, etc. */
    printf("Testing 'array_delete_index()'...\n");
    array_delete_index(arr4, 1);
    printf("--> count: %d\n", arr4->count);

    /* TODO: valgrind is accusing a memory leak in array_clone */
    printf("Testing 'array_clone()'...\n");
    arr3 = array_clone(arr1);
    printf("--> arr1->size: %i, arr1->v address: %p\n", arr1->size, arr1->v);
    printf("--> arr3->size: %i, arr3->v address: %p\n", arr3->size, arr3->v);
    printf("--> arr1->v[0]: %i, arr1->v[0] address: %p\n", *(int *)arr1->v[0], arr1->v[0]);
    printf("--> arr3->v[0]: %i, arr3->v[0] address: %p\n", *(int *)arr3->v[0], arr3->v[0]);
    printf("--> &i: %p\n", i);

    printf("Testing 'array_trim()'...\n");
    printf("--> arr1->size: from %d", arr1->size);
    array_trim(arr1);
    printf(" to %d\n", arr1->size);
    printf("--> arr4->size: from %d", arr4->size);
    array_trim(arr4);
    printf(" to %d\n", arr4->size);

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

int main (int argc, char *argv[])
{
     mem_test();
     array_test();

     return 0;
}

