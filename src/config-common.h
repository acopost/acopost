#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_NICE 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_DRAND48 1
#define HAVE_SRAND48 1
#endif

#ifndef CONFIG_COMMON_H
#define CONFIG_COMMON_H

#include <stdio.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_VALUES_H
#include <values.h>
#endif
#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F
#endif
#ifndef DBL_MAX
#define DBL_MAX 1.7976931348623158e+308
#endif

#ifndef MAXFLOAT
#define MAXFLOAT FLT_MAX
#endif

#ifndef MAXDOUBLE
#define MAXDOUBLE DBL_MAX
#endif

#include <stdlib.h>

#ifndef HAVE_DRAND48
static double drand48(void)
{
	double ret = rand();
	ret /= RAND_MAX;
	return ret;
}
#else
double drand48(void);
#endif

#ifndef HAVE_SRAND48
static void srand48(long int seedval)
{
	srand(seedval);
}
#else
void srand48(long int seedval);
#endif

#ifndef HAVE_NICE
static int nice(int incr)
{
	fprintf(stderr, "nice() not implemented. Returning -1.\n");
	return -1;
}
#else
int nice(int incr);
#endif

#ifndef HAVE_STRDUP
#include <string.h>
#include <errno.h>
char *strdup(const char *s)
{
	size_t len = strlen(s);
	char* d = malloc(len+1);
	if(d != NULL)
	{
		strcpy(d, s);
	}
	else
	{
		errno = ENOMEM;
	}
	return d;
}
#else
char *strdup(const char *);
#endif

#endif /* CONFIG_COMMON_H */


