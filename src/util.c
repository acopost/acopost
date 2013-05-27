/*
  Some utility functions
  
  Copyright (C) 2001 Ingo Schr√∂der

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#ifdef HAVE_SYS_RESOURCES_H
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
#ifdef HAVE_SYS_RESOURCES_H
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
  exit(0);
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
int intcompare(const void *ip, const void *jp)
{
  int i = *((int *)ip);
  int j = *((int *)jp);

  if (i > j) { return 1; }
  if (i < j) { return -1; }
  return 0;
}

/* ------------------------------------------------------------ */
FILE *try_to_open(char *name, char *mode)
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
char *tokenizer(char *s, char *sep)
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
char *ftokenizer(FILE *ff, char *sep)
{
#define BS (16*1024-1)
  static char b[BS+1];
  static FILE *f=NULL;
  static char *s;
  static char *t;  
  char *r;
  int len;
  
  if (ff) { f=ff; s=b; t=b; *t='\0'; }

  s+=strspn(s, sep);
  if (!*s)
    {
      t=b+fread(b, 1, BS, f); 
      *t='\0';
      s=b+strspn(b, sep);
      if (!*s) { return NULL; }
    }
  /* s points to start of token
     t points to \0 at end of valid buffer
   */
  len=strcspn(s, sep);
  if (s+len==t)
    {
      /* current token is limited by end of valid buffer */
      memmove(b, s, len);
      t=b+len;
      s=b;
      t+=fread(t, 1, BS-len, f); 
      *t='\0';
      len=strcspn(s, sep);
    }
  s[len]='\0';
  r=s;
  s+=len;
  if (s!=t) { s++; }
  return r;
}

/* ------------------------------------------------------------ */
char *reverse(char *s)
{
  static int csize=5;
  static char *b=NULL;
  int sl=strlen(s);
  int oldcsize=csize;
  int i;
  
  if (!b) { b=(char *)mem_malloc(csize); }
  for (;csize<sl;csize*=2) { }
  if (csize!=oldcsize) { b=(char *)mem_realloc(b, csize); }
  b[sl]='\0';
  for (i=0;i<sl; i++) { b[i]=s[sl-i-1]; }
  return b;
}

/* ------------------------------------------------------------ */
char *freadline(FILE *f)
{
  static int csize=5;
  static char *b=NULL;
  char *s;
  int sl;

  if (!b) { b=(char *)mem_malloc(csize); }

  s=fgets(b, csize, f);
  if (!s) { return NULL; }
  sl=strlen(s);
/*   fprintf(stderr, ">> csize=%d sl=%d s[sl-1]=%d s=\"%s\" %p b=\"%s\"\n",  */
/* 	  csize, sl, s[sl-1], s, s, b); */
  while (s[sl-1]!='\n')
    {
      int oldsize=csize;
      csize*=2;
      b=(char *)mem_realloc(b, csize);
/*       fprintf(stderr, ">> fgets at %d %d\n", oldsize-1, b[oldsize-1]); */
      s=fgets(b+oldsize-1, oldsize+1, f);
      if (!s) { return b; }
      sl=strlen(s);
/*       fprintf(stderr, ">> csize=%d sl=%d s[sl-1]=%d s=\"%s\" %p b=\"%s\"\n",  */
/* 	      csize, sl, s[sl-1], s, s, b); */
    }
  s[sl-1]='\0';
  return b;
}

/* ------------------------------------------------------------ */
char *register_string(char *s)
{
  static hash_pt table=NULL;
  char *t=NULL;

  if (!s) { return s; } 
  if (!table) 
    { table=hash_new(1000, 0.6, hash_string_hash, hash_string_equal); }
  else { t=hash_get(table, s); if (t) { return t; } }

  t=strdup(s);
  hash_put(table, t, t);
  return t;
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
char mytolower(char c)
{
  static char *UC="ABCDEFGHIJKLMNOPQRSTUVWXYZƒ÷‹";
  static char *LC="abcdefghijklmnopqrstuvwxyz‰ˆ¸";

  char *t=strchr(UC, c);
  return t ? LC[t-UC] : c;
}

/* ------------------------------------------------------------ */
char *lowercase(char *s)
{
  static char b[4096];
  int i;

  for (i=0; s[i] && i<4096; i++) { b[i]=mytolower(s[i]); }
  if (i==4096) { error("lowercase static buffer exceeded\n"); }
  b[i]='\0';
  return b;
}

/* ------------------------------------------------------------ */
int common_prefix_length(char *s, char *t)
{
  int c=0;
  
  if (!s || !t) { return 0; }
  while (*s && *s==*t) { s++; t++; c++; }
  return c;
}

/* ------------------------------------------------------------ */
int common_suffix_length(char *s, char *t)
{
  char *a;
  char *b;
  int c=0;
  
  if (!s || !t || !*s || !*t) { return 0; }
  a=s+strlen(s)-1;
  b=t+strlen(t)-1;
  while (a>=s && b>=t && *a==*b) { a--; b--; c++; }

  return c;
}

/* ------------------------------------------------------------ */
/* EOF */
