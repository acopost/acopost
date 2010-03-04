/*
  Lexicon data structure

  Copyright (C) 2001, 2002 Ingo Schr�der

  Contact info:
  
    ingo@nats.informatik.uni-hamburg.de
    http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
  
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h> /* strdup/Solaris */
#include <string.h> /* strdup/Linux */
#include <ctype.h> /* islower */
#include <errno.h> 
#include <string.h> /* strerror */
#include "lexicon.h"
#include "hash.h"
#include "array.h"
#include "util.h"
#include "mem.h"

/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
lexicon_pt gl;

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
static lexicon_pt new_lexicon(void)
{
  lexicon_pt l=(lexicon_pt)mem_malloc(sizeof(lexicon_t));
  
  l->tags=array_new(50);
  l->taghash=hash_new(100, .5, hash_string_hash, hash_string_equal);

  l->words=hash_new(5000, .5, hash_string_hash, hash_string_equal);

  return l;
}

/* ------------------------------------------------------------ */
static word_pt new_word(char *s, int not)
{
  word_pt w=(word_pt)mem_malloc(sizeof(word_t));

  w->string=s;
  w->tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(w->tagcount, 0, not*sizeof(int));
  w->sorter=(int *)mem_malloc(not*sizeof(int));
  memset(w->sorter, -1, not*sizeof(int));

  return w;
}

/* ------------------------------------------------------------ */
static void delete_word(word_pt w)
{
  mem_free(w->tagcount);
  mem_free(w->sorter);
  mem_free(w);
}

/* ------------------------------------------------------------ */
extern char *tagname(lexicon_pt l, int i)
{ return (char *)array_get(l->tags, i); }

/* ------------------------------------------------------------ */
extern int find_tag(lexicon_pt l, char *t)
{ return (int)hash_get(l->taghash, t)-1; }

/* ------------------------------------------------------------ */
static int register_tag(lexicon_pt l, char *t)
{
  int i=find_tag(l, t);
 
  if (i<0) 
    { 
      t=strdup(t); 
      i=array_add(l->tags, t);
      hash_put(l->taghash, t, (void *)(i+1));
    }
  return i;
}

/* ------------------------------------------------------------ */
static int tagcount_compare(const void *ip, const void *jp)
{
  int i = *((int *)ip), j = *((int *)jp);
  
/*   report(0, "tagcount_compare tc[%d]=%d tc[%d]=%d\n", i, gl->tagcount[i], j, gl->tagcount[j]); */
  if (gl->tagcount[i]<gl->tagcount[j]) { return 1; }
  if (gl->tagcount[i]>gl->tagcount[j]) { return -1; }
  return 0;
}
  
/* ------------------------------------------------------------ */
extern lexicon_pt read_lexicon_file(char *fn)
{
  FILE *f;
  lexicon_pt l=new_lexicon();
  char *s;
  int lno, not;

  if (fn) { l->fname=strdup(fn); f=try_to_open(fn, "r"); }
  else { l->fname="STDIN"; f=stdin; }
  for (s=freadline(f); s; s=freadline(f)) 
    {
      for (s=strtok(s, " \t"), s=strtok(NULL, " \t");
	   s;
	   s=strtok(NULL, " \t"), s=strtok(NULL, " \t"))
	{ (void)register_tag(l, s); }
    }
  not=array_count(l->tags);
  l->tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(l->tagcount, 0, not*sizeof(int));
  
  if (fseek(f, 0, SEEK_SET)) { error("can't rewind file \"%s\"\n", fn); }
  
  for (lno=1, s=freadline(f); s; lno++, s=freadline(f)) 
    {
      int cnt, i;
      word_pt wd, old;

      s=strtok(s, " \t");
      if (!s) { report(1, "can't find word (%s:%d)\n", fn, lno); continue; }
      s=register_string(s);

      wd=new_word(s, not);
      old=hash_put(l->words, s, wd);
      if (old)
	{
	  report(1, "duplicate dictionary entry \"%s\" (%s:%d)\n", s, fn, lno);
	  delete_word(old);
	}

      for (i=0, s=strtok(NULL, " \t"); s;  i++, s=strtok(NULL, " \t"))
	{
	  int ti=find_tag(l, s);
	  
	  if (ti<0)
	    { report(0, "invalid tag \"%s\" (%s:%d)\n", s, fn, lno); continue; }
	  s=strtok(NULL, " \t");
	  if (!s || 1!=sscanf(s, "%d", &cnt))
	    { report(1, "can't find tag count (%s:%d)\n", fn, lno); continue; }
	  wd->count+=cnt;
	  wd->tagcount[ti]=cnt;
	  if (i==0) { wd->defaulttag=ti; }
	  wd->sorter[i]=ti;
	  l->tagcount[ti]+=cnt;
	}
    }
  l->sorter=(int *)mem_malloc(not*sizeof(int));
  for (lno=0; lno<not; lno++) { l->sorter[lno]=lno; }
  gl=l; qsort(l->sorter, not, sizeof(int), tagcount_compare);
  l->defaulttag=l->sorter[0];
  
  return l;
}

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
