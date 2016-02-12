/*
  Some utility functions
  
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

/* ------------------------------------------------------------ */
#include "config-common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include "hash.h"
#include "util.h"
#include "mem.h"

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* 0 errors only, 1 normal, ..., 6 really chatty */
int verbosity=1;

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
unsigned long used_ms()
{
#ifdef HAVE_SYS_RESOURCE_H
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru)) { report(0, "Can't get rusage: %s\n", strerror(errno)); }

  return 
    (ru.ru_utime.tv_sec+ru.ru_stime.tv_sec)*1000+
    (ru.ru_utime.tv_usec+ru.ru_stime.tv_usec)/1000;
#else
  fprintf(stderr, "used_ms() implementation missing, returning 0ms.\n");
  return 0;
#endif
}

/* ------------------------------------------------------------ */
void error(char *format, ...)
{
  va_list ap;

  fprintf(stderr, "[%9ld ms::%d] ", used_ms(), verbosity);
  fprintf(stderr, "ERROR: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

/* ------------------------------------------------------------ */
void report(int mode, char *format, ...)
{
  va_list ap;

  if (abs(mode)>verbosity) { return; }

  if (mode>=0) { fprintf(stderr, "[%9ld ms::%d] ", used_ms(), verbosity); } 
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/* ------------------------------------------------------------ */
void print_progress(int mode)
{
  static int c=0;
  static char *wheel="|/-\\|/-\\";

  if (abs(mode)>verbosity) { return; }

  report(mode, "%c\r", wheel[c]);
  if (!wheel[++c]) { c=0; }
}

/* ------------------------------------------------------------ */
int intcmp(const void *ip, const void *jp)
{
  return *((int *)ip) - *((int *)jp);
}

/* ------------------------------------------------------------ */
FILE *try_to_open(const char *name, char *mode)
{
  FILE *f;

  f=fopen(name, mode);
  if (!f) { error("can't open file \"%s\" \"%s\": %s\n", name, mode, strerror(errno)); }
  return f;
}

/* ------------------------------------------------------------ */
char *acopost_basename(char *name, char *s)
{
#ifndef MAXPATHLEN
#define MAXPATHLEN 4096
#endif
  static char b[MAXPATHLEN];
  int tl, sl;
  char *t;
  
  t=strrchr(name, '/');
  if (!t) { t=name; } else { t++; }
  tl=strlen(t);
  if (tl+1>=MAXPATHLEN) { error("acopost_basename: \"%s\" too long\n", t); }
  b[0]='\0';
  strncat(b, t, MAXPATHLEN-1);
  if (s && tl>=(sl=strlen(s)) && !strcmp(t+tl-sl, s)) { b[tl-sl]='\0'; }
  return b;
}

/* ------------------------------------------------------------ */
char *tokenizer(char *s, const char *sep)
{
  static char c;
  static char *last=NULL;
  static int len=0;
  
  if (!s)
    {
      if (!last) { return NULL; }
      last[len]=c;
      last+=len;
      s=last;
    }

  last=s+strspn(s, sep);
  if (!*last) { last=NULL; return NULL; }
  len=strcspn(last, sep);
  c=last[len];
  last[len]='\0';
/*   report(-1, "last=\"%s\" len %d strspn(\"%s\", \"%s\")==%d\n", last, len, s, sep, strspn(s, sep)); */

  return last;
}

/* ------------------------------------------------------------ */
char *reverse(const char *s, char**buffer, size_t *n)
{
  size_t sl;
  size_t i;
  if(!s) {
	  return NULL;
  }
  sl=strlen(s);
  if (!*buffer) {
	  *buffer=(char *)mem_malloc(sl+1);
	  *n = sl+1;
  }
  if (*n<=sl) {
	  *buffer=(char *)mem_realloc(*buffer, sl+1);
	  *n = sl+1;
  }
  (*buffer)[sl]='\0';
  for (i=0; i<sl; i++) {
	  (*buffer)[i]=s[sl-i-1];
  }
  return *buffer;
}

/* ------------------------------------------------------------ */
static char *UC="ABCDEFGHIJKLMNOPQRSTUVWXYZ\xc4\xd6\xdc";
static char *LC="abcdefghijklmnopqrstuvwxyz\xe4\xf6\xfc";

/* ------------------------------------------------------------ */
size_t uppercase_prefix_length(char *s)
{
  return strspn(s, UC);
}

/* ------------------------------------------------------------ */
char* get_first_uppercase(char *s)
{
  return strpbrk(s, UC);
}

/* ------------------------------------------------------------ */
int is_uppercase(int c)
{
	return (int)(size_t)strchr(UC, c);
}


/* ------------------------------------------------------------ */
char mytolower(char c)
{
  char *t=strchr(UC, c);
  return t ? LC[t-UC] : c;
}

/* ------------------------------------------------------------ */
char *lowercase(char *s, char**buffer, size_t *n)
{
  size_t sl;
  size_t i;
  if(!s) {
	  return NULL;
  }
  sl=strlen(s);
  if (!*buffer) {
	  *buffer=(char *)mem_malloc(sl+1);
	  *n = sl+1;
  }
  if (*n<=sl) {
	  *buffer=(char *)mem_realloc(*buffer, sl+1);
	  *n = sl+1;
  }
  (*buffer)[sl]='\0';
  for (i=0; i<sl; i++) {
	  (*buffer)[i]=mytolower(s[i]);
  }
  return *buffer;
}

ssize_t readdelim(char **lineptr, size_t  *n, int delim, FILE *stream)
{
	size_t tmpn;
	char *tmplineptr;
	ssize_t i;
	int c;
	if(feof(stream)) {
		return -1;
	}

	if(*n<=0) {
		tmpn = 32;
		tmplineptr = (char*) realloc(*lineptr, tmpn);
		if(tmplineptr!=NULL) {
			*n = tmpn;
			*lineptr = tmplineptr;
		} else {
			return -1;
		}
	}
	if (!*lineptr) {
		*lineptr = (char*) malloc(*n);
		if(*lineptr == NULL) {
			return -1;
		}
	}

	i = 0;
	while((c=getc(stream))!=EOF)
	{
		(*lineptr)[i]=(unsigned char)c;
		i++;
		if(c == delim) {
			break;
		}
		if(i >= (ssize_t) (*n))
		{
			tmpn = (*n) *2;
			tmplineptr = (char*) realloc(*lineptr, tmpn);
			if(tmplineptr!=NULL) {
				*n = tmpn;
				*lineptr = tmplineptr;
			} else {
				return -1;
			}
		}
	}
	if(ferror(stream)) {
		return -1;
	}
	if(i >= (ssize_t) (*n))
	{
		tmpn = (*n) +1;
		tmplineptr = (char*) realloc(*lineptr, tmpn);
		if(tmplineptr!=NULL) {
			*n = tmpn;
			*lineptr = tmplineptr;
		} else {
			return -1;
		}
	}
	(*lineptr)[i]= '\0';
	return i;
}

ssize_t readline(char **lineptr, size_t  *n, FILE *stream) {
	return readdelim(lineptr, n, '\n', stream);
}

/* ------------------------------------------------------------ */
char *substr(char *s, int pos, int length)
{
  static char b[4096];
  int i;

  if (abs(length)>=4096) { error("substr static buffer exceeded\n"); }

  if (length>=0)
    {
      for (i=0; length>0 && s[pos+i]; length--, i++) { b[i]=s[pos+i]; }
      b[i]='\0';
    }
  else
    {
      b[abs(length)]='\0';
      for (i=0; length<0 && s[pos+length+1]; length++, i++) { b[i]=s[pos+length+1]; }
    }

  return b;
}

/* ------------------------------------------------------------ */
size_t common_prefix_length(const char *s, const char *t)
{
  size_t c=0;
  
  if (!s || !t) { return 0; }
  while (*s && *s==*t) { s++; t++; c++; }
  return c;
}

/* ------------------------------------------------------------ */
size_t common_suffix_length(const char *s, const char *t)
{
  const char *a;
  const char *b;
  size_t c=0;
  
  if (!s || !t || !*s || !*t) { return 0; }
  a=s+strlen(s)-1;
  b=t+strlen(t)-1;
  while (a>=s && b>=t && *a==*b) { a--; b--; c++; }

  return c;
}

/* ------------------------------------------------------------ */
/* EOF */
