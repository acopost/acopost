/*
  Copyright (c) 2001-2002, Ingo Schröder
  Copyright (c) 2007-2013, ACOPOST Developers Team
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

#ifndef CONFIG_COMMON_H
#define CONFIG_COMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_NICE 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_DRAND48 1
#define HAVE_SRAND48 1
#endif

static const char* version_copyright_banner = "  ACOPOST 2.0.0 <https://github.com/acopost/acopost>\n  Copyright (c) 2007-2016, ACOPOST Developers Team\n  Copyright (c) 2001-2002, Ingo Schröder";

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


