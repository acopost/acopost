#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "array.h"
#include "hash.h"
#include "mem.h"
#include "primes.h"
#include "util.h"

int main (int argc, char *argv[])
{
  int a = 13, b=14;
  int *ptr = &a, *ptr2 = &b;
  int *temp;

  hash_pt myhash = hash_new(100, .7, hash_string_hash, hash_string_equal);

  char *str = "este é um teste";

  array_pt array = array_new(10);
  array_pt array_str = array_new(10);
  array_add(array, ptr);
  array_add(array_str, str);

  temp = (int*)array_get(array, 0);

  printf("prova 1: %i\n", *temp);
  printf("prova 2: %i\n", *((int *)array_get(array, 0)) );

  printf("prova 3: %s\n", (char *)array_get(array_str, 0) );

  hash_put(myhash, ptr, ptr2);
  printf("%i\n", *((int *)hash_get(myhash, ptr)) );

  return 0;
}

