/*
  Example-based Tagger

  Copyright (c) 2001-2002, Ingo Schröder
  Copyright (c) 2007-2016, ACOPOST Developers Team
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
#include <stddef.h> /* for ptrdiff_t and size_t. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strtok */
#endif
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include "hash.h"
#include "array.h"
#include "util.h"
#include "mem.h"
#include "sregister.h"
#include "iregister.h"

/* ------------------------------------------------------------ */

#if 0
#define DEBUG(mode, msg...) report(mode, msg)
/*
  see http://gcc.gnu.org/onlinedocs/gcc-3.0.1/gcc_5.html#SEC82
  #define DEBUG(mode, msg...) fprintf (mode, msg, ## __VA_ARGS__)
*/
#else
#define DEBUG(mode, msg...) 
#endif

/* ------------------------------------------------------------ */

#define MODE_TAG 1
#define MODE_TEST 2

typedef struct option_s
{
  char ch;
  char *usage;
} option_t;
typedef option_t *option_pt;

typedef struct globals_s
{
  int mode;     /* mode: tagging, training, testing */
  char *cmd;    /* command name */   
  char *kf;     /* wtree file for known words */
  char *uf;     /* wtree file for unknown words */
  char *df;     /* dictionary file name */
  char *rf;     /* name of file to tag/test */
} globals_t;
typedef globals_t *globals_pt;

#define FT_TAG 1
#define FT_CLASS 2
#define FT_WORD 3
#define FT_LETTER 4
#define FT_CAP 5
#define FT_HYPHEN 6
#define FT_NUMBER 7
#define FT_INTER 8

typedef struct feature_s
{
  ptrdiff_t type;          /* type of feature, 0 WORD, 1 TAG ... */
  ptrdiff_t arg1;          /* 1st para, e. g., WORD[-1] */
  ptrdiff_t arg2;          /* 2nd para, e. g., LETTER[0,-1] */
  char *string;
  double weight;     /* weight */
  hash_pt v2i;       /* string -> index+1 */
  array_pt values;   /* list of values */
} feature_t;
typedef feature_t *feature_pt;

typedef struct value_s
{
  char *string;      /* literal value */
  size_t count;      /* how often have we seen this */
  int *tagcount;     /* array of ints */
} value_t;
typedef value_t *value_pt;

typedef struct wtree_s
{
  ptrdiff_t defaulttag;     /* tag with largest tagcount[] */
  int *tagcount;      /* index -> count */
  char *id;
  feature_pt feature; /* feature corresponding to node */
  array_pt children;  /* index -> wtree */
} wtree_t;
typedef wtree_t *wtree_pt;

typedef struct word_s
{
  char *string;      /* grapheme */
  size_t count;      /* total number of occurances */
  int *tagcount;     /* maps tag index -> no. of occurances */
  ptrdiff_t defaulttag;    /* most frequent tag index */
  char *aclass;      /* ambiguity class */
} word_t;
typedef word_t *word_pt;

typedef struct model_s
{
  iregister_pt tags;  /* lookup table tags */

  array_pt features;  /* list of sorted features */

  hash_pt dictionary; /* dictionary: string->int (best tag) */
  wtree_pt known;     /* weighted tree for known words */
  wtree_pt unknown;   /* weighted tree for known words */
  sregister_pt strings;
} model_t;
typedef model_t *model_pt;

/* ------------------------------------------------------------ */
char *banner=
"Example-based Tagger (c) Ingo Schröder and others, http://acopost.sf.net/";

option_t ops[]={
  { 't', "-t    test mode" },
  { 'v', "-v v  verbosity [1]" },
  { '\0', NULL },
};

/* ------------------------------------------------------------ */
static globals_pt new_globals(globals_pt old)
{
  globals_pt g=(globals_pt)mem_malloc(sizeof(globals_t));

  if (old) { memcpy(g, old, sizeof(globals_t)); return g; }

  g->mode=MODE_TAG;
  g->cmd=NULL;
  g->kf=g->uf=g->df=g->rf=NULL;
  
  return g;
}

/* ------------------------------------------------------------ */
static model_pt new_model(void)
{
  model_pt m=(model_pt)mem_malloc(sizeof(model_t));
  memset(m, 0, sizeof(model_t));

  m->features=array_new(32);

  return m;
}

/* ------------------------------------------------------------ */
static word_pt new_word(char *s, size_t cnt, size_t not)
{
  word_pt w=(word_pt)mem_malloc(sizeof(word_t));

  w->string=s;
  w->count=cnt;
  w->tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(w->tagcount, 0, not*sizeof(int));

  return w;
}

/* ------------------------------------------------------------ */
static void delete_word(word_pt w)
{
  mem_free(w->tagcount);
  mem_free(w);
}

/* ------------------------------------------------------------ */
static feature_pt new_feature(void)
{
  feature_pt f=(feature_pt)mem_malloc(sizeof(feature_t));
  memset(f, 0, sizeof(feature_t));

  f->weight=0.0;
  f->v2i=hash_new(50, .5, hash_string_hash, hash_string_equal);
  f->values=array_new(32);
  
  return f;
}

/* ------------------------------------------------------------ */
static wtree_pt new_wtree(size_t not)
{
  static char *foo42="foo42";
  wtree_pt t=(wtree_pt)mem_malloc(sizeof(wtree_t));
  memset(t, 0, sizeof(wtree_t));

  t->id=foo42;
  t->tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(t->tagcount, 0, not*sizeof(int));
  t->children=array_new(32);

  return t;
}

/* ------------------------------------------------------------ */
static void usage(globals_pt g)
{
  size_t i;
  report(-1, "\n%s\n\n", banner);
  report(-1, "Usage: %s OPTIONS knownwtree unknownwtree dictionaryfile [inputfile]\n", g->cmd);
  report(-1, "where OPTIONS can be\n\n");
  for (i=0; ops[i].usage; i++)
    { report(-1, "  %s\n", ops[i].usage); }
  report(-1, "\n");
}

/* ------------------------------------------------------------ */
static void get_options(globals_pt g, int argc, char **argv)
{
  char c;

  while ((c=getopt(argc, argv, "tv:"))!=EOF)
    {
      switch (c)
	{
	case 't':
	  g->mode=MODE_TEST;
	  break;
	case 'v':
	  if (1!=sscanf(optarg, "%d", &verbosity))
	    { error("invalid verbosity \"%s\"\n", optarg); }
	  break;
	default:
	  error("unknown option \"-%c\"\n", c);
	  break;
	}
    }

  if (optind+2>=argc) { usage(g); error("too few arguments\n"); }
  g->kf=strdup(argv[optind]);
  g->uf=strdup(argv[optind+1]);
  g->df=strdup(argv[optind+2]);
  if (optind+3<argc && strcmp("-", argv[optind+3]))
    { g->rf=strdup(argv[optind+3]); }
}


/* ------------------------------------------------------------ */
static void read_dictionary_file(const char*fn, model_pt m)
{
  FILE *f=try_to_open(fn, "r");
  char *s, *rs;
#define BLEN 8000
  char b[BLEN];
  size_t lno, not;
  size_t no_token=0;
  ssize_t r;
  char *buf = NULL;
  size_t n = 0;
  unsigned long tmp;
  
  /* first pass through file: just get the tag */
  m->tags=iregister_new(128);
  lno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      for (s=tokenizer(s, " \t"), s=tokenizer(NULL, " \t");
	   s;
	   s=tokenizer(NULL, " \t"), s=tokenizer(NULL, " \t"))
	{
      iregister_add_name(m->tags, s);
        }
    }
  not=iregister_get_length(m->tags);
  report(2, "found %d tags in \"%s\"\n", not-1, fn);

  /* rewind file */
  if (fseek(f, 0, SEEK_SET)) { error("can't rewind file \"%s\"\n", fn); }

  /* second pass through file: collect details */
  m->dictionary=hash_new(5000, .5, hash_string_hash, hash_string_equal);

  lno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if(r==0) { continue; }
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      size_t cnt;
      word_pt wd, old;
      
      s=tokenizer(s, " \t");
      if (!s) { report(1, "can't find word (%s:%lu)\n", fn, (unsigned long) lno); continue; }
      rs=(char*)sregister_get(m->strings,s);
      wd=new_word(rs, 0, not);
      old=hash_put(m->dictionary, rs, wd);
      if (old)
	{
	  report(1, "duplicate dictionary entry \"%s\" (%s:%lu)\n", s, fn, (unsigned long) lno);
	  delete_word(old);
	}
      wd->defaulttag=-1;
      b[0]='*', b[1]='\0';
      for (s=tokenizer(NULL, " \t"); s;  s=tokenizer(NULL, " \t"))
	{
	  ptrdiff_t ti=iregister_get_index(m->tags, s);
	  
	  if (ti<0)
	    { report(0, "invalid tag \"%s\" (%s:%lu)\n", s, fn, (unsigned long) lno); continue; }
	  if (strlen(b)+strlen(s)+2>BLEN)
	    { error("oops, ambiguity class too long (%s:%lu)\n", fn, (unsigned long) lno); }
	  strcat(b, s); strcat(b, "*");
	  s=tokenizer(NULL, " \t");
	  if (!s || 1!=sscanf(s, "%lu", &tmp))
	    { report(1, "can't find tag count (%s:%lu)\n", fn, (unsigned long) lno); continue; }
	  cnt = tmp;
	  wd->count+=cnt;
	  wd->tagcount[ti]=cnt;
	  if (wd->defaulttag<0) { wd->defaulttag=ti; }
	}
      wd->aclass=(char*)sregister_get(m->strings, b);
      no_token+=wd->count;
    }
  report(2, "read %d/%d entries (type/token) from dictionary\n",
	 hash_size(m->dictionary), no_token);
  if(buf!=NULL){
    free(buf);
    buf = NULL;
    n = 0;
  }
  fclose(f);
}

/* ------------------------------------------------------------ */
static feature_pt parse_feature(model_pt m, char *t)
{
  feature_pt f=new_feature();
  
  t=strtok(t, " \t");
  if (!t) { return NULL; }
  f->string=strdup(t);
  t=strtok(NULL, " \t");
  if (!t) { return NULL; }
  if (1!=sscanf(t, "%lf", &f->weight)) { return NULL; }

  f->type=f->arg1=f->arg2=-1;
  if (1==sscanf(f->string, "TAG[%td]=", &f->arg1))
    { f->type=FT_TAG; }
  else if (1==sscanf(f->string, "CLASS[%td]=", &f->arg1))
    { f->type=FT_CLASS; }
  else if (1==sscanf(f->string, "WORD[%td]=", &f->arg1))
    { f->type=FT_WORD; }
  else if (2==sscanf(f->string, "LETTER[%td,%td]=", &f->arg1, &f->arg2))
    { f->type=FT_LETTER; }
  else if (1==sscanf(f->string, "CAP[%td]=", &f->arg1))
    { f->type=FT_CAP; }
  else if (1==sscanf(f->string, "HYPHEN[%td]=", &f->arg1))
    { f->type=FT_HYPHEN; }
  else if (1==sscanf(f->string, "NUMBER[%td]=", &f->arg1))
    { f->type=FT_NUMBER; }
  else if (1==sscanf(f->string, "INTER[%td]=", &f->arg1))
    { f->type=FT_INTER; }
  else { error("can't parse feature \"%s\"!\n", f->string); }

/*   report(0, "found feature %s type %td arg1 %td arg2 %td\n", f->string, f->type, f->arg1, f->arg2); */
  return f;
}

/* ------------------------------------------------------------ */
static ptrdiff_t find_feature_value(feature_pt f, const char *s)
{
	ptrdiff_t fi=((ptrdiff_t)hash_get(f->v2i, (char *)s))-1;
  DEBUG(3, "find_feature_value %s %s ==> %td\n", f->string, s, fi); 
  return fi;
}

/* ------------------------------------------------------------ */
static size_t register_feature_value(feature_pt f, char *s, sregister_pt strings)
{
  ptrdiff_t i=find_feature_value(f, s);
  if (i<0)
    {
      s=(char*)sregister_get(strings, s);
      i=array_add(f->values, s);
      hash_put(f->v2i, s, (void *)(i+1));
    }

  return i;
}

/* ------------------------------------------------------------ */
static size_t find_feature_value_from_sentence(model_pt m, feature_pt f, char **ws, int *ts, ptrdiff_t i, size_t wno)
{
  ptrdiff_t rp=i+f->arg1;  
  word_pt w;

  DEBUG(2, "find_feature_value_from_sentence: f=%s rp=%td i=%td arg1=%td arg2=%td\n", f->string, rp, i, f->arg1, f->arg2);
  
  if (rp<0 || rp>=wno) { return find_feature_value(f, "*BOUNDARY*"); }

  w=hash_get(m->dictionary, ws[rp]);
  switch (f->type)
    {
    case FT_TAG:
      return find_feature_value(f, iregister_get_name(m->tags, ts[rp]));
    case FT_CLASS:
      return !w ? -1 : find_feature_value(f, w->aclass);
    case FT_WORD:
      {
	ptrdiff_t fi=find_feature_value(f, ws[rp]);
	return fi<0 ? find_feature_value(f, "*RARE*") : fi;
      }
    case FT_LETTER:
      {
	size_t slen=strlen(ws[rp]);
	if (slen<abs(f->arg2)) { return find_feature_value(f, "*NONE*"); }
	else if (f->arg2<0) { return find_feature_value(f, substr(ws[rp], slen+f->arg2, -1)); }
	else { return find_feature_value(f, substr(ws[rp], f->arg2-1, 1)); }
      }
    case FT_CAP:
      {
	char tmp[2]="X";
	size_t slen=strlen(ws[rp]);
	size_t clen=strspn(ws[rp], "ABCDEFGHIJKLMNOPQRSTUVWXY\xc4\xd6\xdc");
	tmp[0]= slen==clen ? '2' : (clen>0 ? '1' : '0');
	return find_feature_value(f, tmp);
      }
    case FT_HYPHEN:
      {
	char tmp[2]="0";
	if (strchr(ws[rp], '-')) { tmp[0]='1'; }
	return find_feature_value(f, tmp);
      }
    case FT_NUMBER:
      {
	char tmp[2]="X";
	size_t slen=strlen(ws[rp]);
	char *dindex=strpbrk(ws[rp], "0123456789");
	size_t clen=strspn(ws[rp], "0123456789,.");
	size_t dlen=strspn(ws[rp], "0123456789");
	tmp[0]= slen==dlen ? '3' : ( slen==clen ? '2' : (dindex ? '1' : '0'));
	return find_feature_value(f, tmp);
      }
    case FT_INTER:
      {
	char tmp[2]="0";
	if (strspn(ws[rp], ",.;?!:")==strlen(ws[rp])) { tmp[0]='1'; }
	return find_feature_value(f, tmp);
      }
    default:
      error("unknown feature type %td\n", f->type);
    }
  return -1;
}

/* ------------------------------------------------------------ */
static void prune_wtree(model_pt m, wtree_pt t)
{
  size_t i;
  size_t not=iregister_get_length(m->tags);

  if (!t) { return; }
  for (i=0; i<array_count(t->children); i++)
    {
      wtree_pt child=array_get(t->children, i);
      size_t j;
      if (!child) { continue; }
      for (j=0; j<not; j++)
	{
	  t->tagcount[j]+=child->tagcount[j];
	  if (t->tagcount[t->defaulttag]<t->tagcount[j]) { t->defaulttag=j; }
	}
    }
  /* TODO: really prune the tree */
}

/* ------------------------------------------------------------ */
static wtree_pt read_wtree(model_pt m, const char *fname)
{
  size_t lno, cl=0, non=1, fos=array_count(m->features);
  ssize_t fno;
  FILE *f=try_to_open(fname, "r");
  wtree_pt root=new_wtree(iregister_get_length(m->tags));
  wtree_pt *ns;
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;
  int in_header = 1;

  /* first read list of features */
  lno = 0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      /* skip over empty lines and comments in header */
      if(in_header) {
	      if((r > 1) && (s[0]=='#' && s[1]=='#')) { continue; }
	      if((r == 1) && (s[0]=='\0')) { continue; }
	      in_header = 0;
      }
      if((r == 1) && (s[0]=='\0')) { break; }
      feature_pt ft=parse_feature(m, s);
      if (!ft) { error("%s:%zd: can't parse feature \"%s\"\n", fname, lno, s); }
      array_add(m->features, ft);
    }

  fno=array_count(m->features)-fos;
  if (fno<=0) { error("%s:%zd: no features found\n", fname, lno); }
  root->feature=array_get(m->features, fos);
  
  /* this is the separating blank line */
  if (!s) { error("%s:%zd: format error\n", fname, lno); }

  ns=(wtree_pt *)mem_malloc((fno+1)*sizeof(wtree_pt));
  memset(ns, 0, (fno+1)*sizeof(wtree_pt));
  ns[0]=root;
  
  /* now parse the tree */
  cl = 1;
  lno = 0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      char *t;
      size_t i, l;
      wtree_pt mom, wt;
      
      if (!*s) { continue; }
      for (l=1; *s=='\t'; s++, l++) { /* nada */ }
      if (l>cl+1)
	{ error("%s:%zd: skip of level %zd>%zd+1\n", fname, lno, l, cl); }
      for (i=cl; i>=l; i--) { prune_wtree(m, ns[i]); }
      cl=l;
      mom=ns[l-1];

      t=strtok(s, " \t");
      if (!t) { error("%s:%zd: can't read value\n", fname, lno); }

      i=register_feature_value(mom->feature, t, m->strings);
      /* i is index of value, wt is current node */
/*       report(-1, "t=%s l=%zd cl=%zd mom->feature=%s i=%zd\n", t, l, cl, mom->feature->string, i); */
      
      if (i<array_count(mom->children) && array_get(mom->children, i))
	{ error("%s:%zd: duplicate feature value %zd %zd\n", fname, lno, i, array_count(mom->children)); }

      wt=new_wtree(iregister_get_length(m->tags));
      non++;
      if (l<fno) { wt->feature=array_get(m->features, l+fos); }
      array_set(mom->children, i, wt);
      for (t=strtok(NULL, " \t"); t; t=strtok(NULL, " \t"))
	{
	  size_t c, ti=iregister_add_name(m->tags, t);

	  /* leaf node */
	  t=strtok(NULL, " \t");
	  if (!t) { error("%s:%lu: can't find tag count\n", fname, (unsigned long) lno); }
	  if (1!=sscanf(t, "%lu", &c))
	    { error("%s:%lu: tag count not a number\n", fname, (unsigned long) lno); }
	  wt->tagcount[ti]=c;
	  if (c>wt->tagcount[wt->defaulttag]) { wt->defaulttag=ti; }
	}
      ns[l]=wt;
    }
  for (fno=cl; fno>=0; fno--) { prune_wtree(m, ns[fno]); }
  mem_free(ns);
  if(buf) {
	  free(buf);
	  buf = NULL;
  }

  report(1, "read wtree with %zd nodes from \"%s\"\n", non, fname);
  
  return root;
}

/* ------------------------------------------------------------ */
static void read_known_wtree(const char *fn, model_pt m)
{ m->known=read_wtree(m, fn); }

/* ------------------------------------------------------------ */
static void read_unknown_wtree(const char *fn, model_pt m)
{ m->unknown=read_wtree(m, fn); }

/* ------------------------------------------------------------ */
void print_wtree(wtree_pt t, int indent)
{
  feature_pt f=t->feature;
  size_t i;
  
  for (i=0; i<array_count(t->children); i++)
    {
      wtree_pt son=(wtree_pt)array_get(t->children, i);
      char *v=(char *)array_get(f->values, i);
      size_t j;
      if (!son) { continue; }
      for (j=0; j<indent; j++) { report(-1, " "); }
      report(-1, "%s\n", v);
      print_wtree(son, indent+2);
    }
}

/* ------------------------------------------------------------ */
static void tagging(const char* fn, model_pt m)
{
  size_t not=iregister_get_length(m->tags), nop=0;
  FILE *f= fn ? try_to_open(fn, "r") : stdin;
  char **words=NULL;
  int *tags=NULL;
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;

  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      char *t;
      size_t i, wno;
      DEBUG(2, "GOT %s\n", s);
      for (wno=0, t=strtok(s, " \t"); t; wno++, t=strtok(NULL, " \t"))
	{
	  if (wno>=nop)
	    {
	      if (nop<=0) { nop=128; }
	      while (wno>=nop) { nop*=2; }
	      words=(char **)mem_realloc(words, nop*sizeof(char *));
	      tags=(int *)mem_realloc(tags, nop*sizeof(int));
	    }
 	  report(4, "word %zd %s\n", wno, t);
	  words[wno]=t;
	}
      /* Now we have the sentence available. */
      for (i=0; i<wno; i++)
	{
	  char *word=words[i];
	  word_pt w=hash_get(m->dictionary, word);
	  wtree_pt tree= w ? m->known : m->unknown;

	  report(4, "word %s is %sknown.\n", word, w ? "" : "un");
	  while (1)
	    {
	      feature_pt f=tree->feature;
	      wtree_pt next;
	      int fi;

	      if (!f) { report(4, "leaf node reached, breaking out\n"); break; }

	      fi=find_feature_value_from_sentence(m, f, words, tags, i, wno);
	      if (fi<0) { report(4, "can't find value, breaking out\n"); break; }
	      if (fi>=array_count(tree->children))
		{ report(4, "can't find child %zd>=%zd, breaking out\n", fi, array_count(tree->children)); break; }
	      next=(wtree_pt)array_get(tree->children, fi);
	      if (!next) { report(4, "can't find child for %zd, breaking out\n", fi); break; }
	      tree=next;
	      {
		size_t j;
		report(4, "  current node %p %td %s :::",
		       tree, tree->defaulttag, (char *)iregister_get_name(m->tags, tree->defaulttag));
		for (j=0; j<not; j++)
		  {
		    if (tree->tagcount[j]<=0) { continue; }
		    report(-4, " %s:%d", (char *)iregister_get_name(m->tags, j), tree->tagcount[j]);
		  }
		report(-4, "\n");
	      }
	    }
	  tags[i]=tree->defaulttag;
	  if (i>0) { printf(" "); }
	  printf("%s %s", word, (char *)iregister_get_name(m->tags, tags[i]));
	  report(4, "OUT: %s %s\n", word, (char *)iregister_get_name(m->tags, tags[i]));
	}
      printf("\n");
    }
  if (words) { mem_free(words); mem_free(tags); }  
  if(buf) {
	  free(buf);
	  buf = NULL;
  }
  if (f!=stdin) { fclose(f); }
}

/* ------------------------------------------------------------ */
void delete_model(model_pt m)
{
  hash_iterator_pt hi;
  void *key;

  /* Delete the tags lookup register. */
  iregister_delete(m->tags);

  /* Delete all words in dictionary. */
  hi = hash_iterator_new(m->dictionary);
  while (NULL != (key = hash_iterator_next_key(hi))) {
    word_pt wd = (word_pt) hash_get(m->dictionary, key);
    if (wd != NULL) {
      delete_word(wd);
    }
  }
  hash_iterator_delete(hi);

  /* Delete the dictionary hash map. */
  hash_delete(m->dictionary);

  /* Free strings register */
  sregister_delete(m->strings);

  /* Delete the model itself. */
  mem_free(m);
}


void delete_globals(globals_pt g)
{
  free(g->cmd);
  free(g->kf);
  free(g->uf);
  free(g->df);
  free(g->rf);
  mem_free(g);
}
/* ------------------------------------------------------------ */
void testing(model_pt m)
{
  char s[4000];
  size_t not=iregister_get_length(m->tags), i;
  
  report(-1, "Enter word followed by features values.\n");
  while (fgets(s, 1000, stdin))
    {
      char *w=strtok(s, " \t\n");
      word_pt wd=hash_get(m->dictionary, w);
      wtree_pt t= wd ? m->known : m->unknown;
      if (wd) { report(-1, "word %s defaulttag %td %s\n", w, wd->defaulttag, (char *)iregister_get_name(m->tags, wd->defaulttag)); }
      while ((w=strtok(NULL, " \t\n")))
	{
	  feature_pt f=t->feature;
	  size_t fi=find_feature_value(f, w);
	  if (fi<0) { report(1, "can't find \"%s\", breaking out\n", w); break; }
	  t=(wtree_pt)array_get(t->children, fi);
	  if (!t) { report(1, "can't find child for %zd, breaking out\n", fi); break; }
	  report(1, "current node %p %td %s :::",
		 t, t->defaulttag, (char *)iregister_get_name(m->tags, t->defaulttag));
	  for (i=0; i<not; i++)
	    {
	      if (t->tagcount[i]>0)
		{ report(-1, " %s:%d", (char *)iregister_get_name(m->tags, i), t->tagcount[i]); }
	    }
	  report(-1, "\n");
	}
    }  
  
}

/* ------------------------------------------------------------ */
int main(int argc, char **argv)
{
  model_pt m=new_model();

  globals_pt g=new_globals(NULL);
  g->cmd=strdup(acopost_basename(argv[0], NULL));
  m->strings = sregister_new(500);
  get_options(g, argc, argv);

  report(1, "\n%s\n\n", banner);

  read_dictionary_file(g->df, m);

  read_known_wtree(g->kf, m);
  read_unknown_wtree(g->uf, m);

  switch (g->mode)
    {
    case MODE_TAG: tagging(g->rf, m); break;
    case MODE_TEST: testing(m); break;
    default: report(0, "unknown mode of operation %d\n", g->mode);
    }

  report(1, "done\n");

  delete_model(m);
  delete_globals(g);

  exit(0);
}

/* ------------------------------------------------------------ */
