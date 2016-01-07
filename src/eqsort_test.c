/*   Date : 2011/03/04, version 2.0                                         */
/*   Copyright (C) 2012, Jingchao Chen                                      */                                
/*   This library was written at Donghua University, China                  */
/*   Contact: chen-jc@dhu.edu.cn or chenjingchao@yahoo.com                  */
/*                                                                          */
/* Permission to use, copy, modify, and distribute this software and its    */
/* documentation with or without modifications and for any purpose and      */
/* without fee is hereby granted, provided that any copyright notices       */
/* appear in all copies and that both those copyright notices and this      */
/* permission notice appear in supporting documentation, and that the       */
/* names of the contributors or copyright holders not be used in            */
/* advertising or publicity pertaining to distribution of the software      */
/* without specific prior permission.                                       */
/*                                                                          */
/* THE CONTRIBUTORS AND COPYRIGHT HOLDERS OF THIS SOFTWARE DISCLAIM ALL     */
/* WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED           */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE         */
/* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT    */
/* OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS   */
/* OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE    */
/* OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE   */
/* OR PERFORMANCE OF THIS SOFTWARE                                          */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "eqsort.h"
#define SIZE_n 450000
#define N_ARRAYS 50
#define ItemType int
static size_t  n=SIZE_n;
ItemType key[SIZE_n];

// compare two member
static int cmp(const void * a,const void *b)
{
	return (*(ItemType *)a - *(ItemType *)b);
}

static int data_cmp(const void * a,const void *b,void *data)
{
	return (*(ItemType *)a - *(ItemType *)b);
}

static void check(const void * a)
{    size_t i;
     for (i=0; i< n-1; i++){
          if (*((ItemType *)a+i)>*((ItemType *)a+i+1))
          {
                printf( "\nThe sequence is not ordered");
                return;
          }
     }
     printf( "\nThe sequence is correctly sorted");
}

int main(int argc, char* args[])
{ 
	size_t  i, j;

	clock_t a, b, c, d;
	printf("\n Adaptive Symmetry Partition Sort \n");
	srand(2007);
	a = clock();
	for(j=0;j<N_ARRAYS;j++)
	{
		for(i=0;i<n;i++) key[i]=rand()%n;
		eqsort((char *)key,n,sizeof(ItemType),data_cmp,NULL);
	}
	b = clock();
	check(key);

	srand(2007);
	c = clock();
	for(j=0;j<N_ARRAYS;j++)
	{
		for(i=0;i<n;i++) key[i]=rand()%n;
		qsort((char *)key,n,sizeof(ItemType),cmp);
	}
	d = clock();
	check(key);

	fprintf(stderr, "qsort %f / adpspsort %f\n", (float)(d-c)/CLOCKS_PER_SEC, (float)(b-a)/CLOCKS_PER_SEC);
	return 0;
}
