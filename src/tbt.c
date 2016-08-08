/*
  Transformation-based tagger

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
#include "options.h"
#include "option_mode.h"
#include <stddef.h> /* for ptrdiff_t and size_t. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strtok */
#endif
#include <ctype.h> /* islower */
#include <math.h> /* sqrt */
#include <errno.h>
#include <getopt.h>
#include "hash.h"
#include "array.h"
#include "util.h"
#include "mem.h"
#include "sregister.h"
#include "iregister.h"

/* ------------------------------------------------------------ */
#ifndef MIN
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a)>(b) ? (a) : (b))
#endif

#define REGISTER_STRING(a) (char*)sregister_get(m->strings,a) 
/* #define REGISTER_STRING(a) strdup(a) */

/* ------------------------------------------------------------ */
typedef struct globals_s
{
  int rawinput; /* flag whether input is in raw format */
  int pos;   /* correctly tagged samples */
  int neg;   /* almost ;-) correctly tagged samples */

  char *rf;     /* rule file name */
  char *ipf;    /* input file name */
  char *plf;    /* preload file name */
  char *tf;     /* template file name */
} globals_t;
typedef globals_t *globals_pt;
#define PRE_TAG 1
#define PRE_TIR 2
#define PRE_WORD 3
#define PRE_PREFIX 4
#define PRE_SUFFIX 5
#define PRE_BOS 6
#define PRE_EOS 7
#define PRE_DIGIT 8
#define PRE_CAP 9
#define PRE_LDC 10
#define PRE_RARE 11

#define PRE_NO 1
#define PRE_SOME 2
#define PRE_ALL 3
#define PRE_ANY 4

/* typedef signed char sint8; */
typedef int sint8;
typedef struct precondition_s
{
  sint8 type;
  sint8 pos;
  union
  {
    int tag;
    char *word;
    struct
    {
      char *prefix;
      size_t length;
    } prefix;
    struct
    {
      char *suffix;
      size_t length;
    } suffix;
    int digit;
    int cap;
  } u;
} precondition_t;
typedef precondition_t *precondition_pt;

#define MAX_NO_PC 10
typedef struct rule_s
{
  char *string;         /* textual representation */
  int lic;           /* last iteration we considered this rule */
  int tag;              /* new tag */ 
  int nop;              /* number of preconditions */
  precondition_t pc[MAX_NO_PC]; /* preconditions */
  int good;
  int bad;
  int delta;            /* good - bad */
} rule_t;
typedef rule_t *rule_pt;

typedef struct sample_s
{
  char *word;         /* word */
  int pos;            /* sample's position in sentence */
  int tag;            /* currently assigned tag */
  int tmptag;         /* temporary tag used while counting */
  int reference;      /* reference tag */
} sample_t;
typedef sample_t *sample_pt;

typedef struct word_s
{
  char *string;      /* grapheme */
  size_t count;      /* total number of occurances */
  int *tagcount;     /* maps tag index -> no. of occurances */
  ptrdiff_t defaulttag;    /* most frequent tag index */
} word_t;
typedef word_t *word_pt;

typedef struct model_s
{
  iregister_pt tags;  /* lookup table tags */

  hash_pt dictionary; /* dictionary: string->int (best tag) */
  array_pt templates; /* rule templates */

  size_t rwt;      /* rare word threshold */
  int defaulttag;     /* most probable tag (unigram) */
  array_pt rules;     /* learned rules, either from file or selected */
  hash_pt rulehash;   /* lookup table for *all* rules generated */
  sregister_pt strings;
} model_t;
typedef model_t *model_pt;

static const char* jokerstring="*"; /* joker string in templates */

/* ------------------------------------------------------------ */
static model_pt new_model(void)
{
  model_pt m=(model_pt)mem_malloc(sizeof(model_t));
  memset(m, 0, sizeof(model_t));

  m->tags=iregister_new(128);
  m->dictionary=hash_new(5000, .5, hash_string_hash, hash_string_equal);
  m->templates=array_new(64);
  m->rules=array_new(1000);
  m->rulehash=hash_new(100000, .7, hash_string_hash, hash_string_equal);
  m->rwt=0;
  m->strings = sregister_new(500);
  return m;
}

/* ------------------------------------------------------------ */
static sample_pt new_sample(void)
{
  sample_pt s=(sample_pt)mem_malloc(sizeof(sample_t));
  memset(s, 0, sizeof(sample_t));
  return s;
}

/* ------------------------------------------------------------ */
static void free_sample(sample_pt sp)
{ mem_free(sp); }

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
void delete_word(word_pt w)
{
  mem_free(w->tagcount);
  mem_free(w);
}

/* ------------------------------------------------------------ */
/* previsously inlined */
static word_pt get_word(model_pt m, char *s)
{ return hash_get(m->dictionary, s); }

/* ------------------------------------------------------------ */
/* previously inlined */
static int is_rare(model_pt m, char *s)
{ word_pt w=get_word(m, s); return !w || w->count <= m->rwt; }

/* ------------------------------------------------------------ */
/* previously inlined */
int word_default_tag(model_pt m, char *s)
{ word_pt w=get_word(m, s); return w ? w->defaulttag : -1; }

/* ------------------------------------------------------------ */
static rule_pt new_rule(rule_pt old)
{
  rule_pt r=(rule_pt)mem_malloc(sizeof(rule_t));
  if (old) { memcpy(r, old, sizeof(rule_t)); }
  else { memset(r, 0, sizeof(rule_t)); }
  return r;
}

/* ------------------------------------------------------------ */
/* previosuly inlined */
static void free_rule(rule_pt r)
{ mem_free(r); }

/* ------------------------------------------------------------ */
static char *precondition2string(model_pt m, precondition_pt pc)
{
#define BSIZE 4096
  static char b[BSIZE];
  int l;
  
  switch (pc->type)
    {
    case PRE_TAG:
      l=snprintf(b, BSIZE, "tag[%d]=%s", pc->pos, pc->u.tag<0 ? jokerstring : iregister_get_name(m->tags, pc->u.tag));
      break;
    case PRE_WORD:
      l=snprintf(b, BSIZE, "word[%d]=%s", pc->pos, pc->u.word ? pc->u.word : jokerstring);
      break;
    case PRE_PREFIX:
      if (pc->u.prefix.prefix)
	{ l=snprintf(b, BSIZE, "prefix[%d]=%s", pc->pos, pc->u.prefix.prefix); }
      else
	{ l=snprintf(b, BSIZE, "prefix[%d]=%zu", pc->pos, pc->u.prefix.length); }
      break;      
    case PRE_SUFFIX:
      if (pc->u.suffix.suffix)
	{ l=snprintf(b, BSIZE, "suffix[%d]=%s", pc->pos, pc->u.suffix.suffix); }
      else
	{ l=snprintf(b, BSIZE, "suffix[%d]=%zu", pc->pos, pc->u.suffix.length); }
      break;      
    case PRE_BOS:
      l=snprintf(b, BSIZE, "bos[%d]", pc->pos);
      break;      
    case PRE_EOS:
      l=snprintf(b, BSIZE, "eos[%d]", pc->pos);
      break;      
    case PRE_DIGIT:
      switch (pc->u.digit)
	{
	case PRE_NO:
	  l=snprintf(b, BSIZE, "digit[%d]=no", pc->pos); break;
	case PRE_SOME:
	  l=snprintf(b, BSIZE, "digit[%d]=some", pc->pos); break;
	case PRE_ALL:
	  l=snprintf(b, BSIZE, "digit[%d]=all", pc->pos); break;
	case PRE_ANY:
	  l=snprintf(b, BSIZE, "digit[%d]=%s", pc->pos, jokerstring); break;
	default:
	  error("invalid DIGIT subtype %d\n", pc->u.digit);
	  l=BSIZE+1;
	}	  
      break;      
    case PRE_CAP:
      switch (pc->u.cap)
	{
	case PRE_NO:
	  l=snprintf(b, BSIZE, "cap[%d]=no", pc->pos); break;
	case PRE_SOME:
	  l=snprintf(b, BSIZE, "cap[%d]=some", pc->pos); break;
	case PRE_ALL:
	  l=snprintf(b, BSIZE, "cap[%d]=all", pc->pos); break;
	case PRE_ANY:
	  l=snprintf(b, BSIZE, "cap[%d]=%s", pc->pos, jokerstring); break;
	default:
	  error("invalid CAP subtype %d\n", pc->u.cap);
	  l=BSIZE+1;
	}	  
      break;      
    case PRE_LDC:
      l=snprintf(b, BSIZE, "ldc");
      break;      
    case PRE_RARE:
      l=snprintf(b, BSIZE, "rare[%d]", pc->pos);
      break;      
    default:
      error("type of precondition %d not known", pc->type);      
      l=BSIZE+1;
    }
    if (l>BSIZE)
      { error("internal error: precondition too long to format\n"); }
  return b;
#undef BSIZE
}

/* ------------------------------------------------------------ */
static char *rule2string(model_pt m, rule_pt r)
{
#define BSIZE 4096
  static char b[BSIZE];
  const char *ts=r->tag<0 ? jokerstring : (char*)iregister_get_name(m->tags, r->tag);
  int tsl=(ssize_t) strlen(ts);
  ptrdiff_t i, bl=BSIZE-1;
  
  b[0]='\0';
  if (bl<=tsl)
    { error("internal error: rule too long to format\n"); }
  strcat(b, ts);
  bl-=tsl;
  for (i=0; i<r->nop; i++)
    {
      char *ps=precondition2string(m, &r->pc[i]);
      int l = (int) strlen(ps);
      if (bl<=l+1)
	{ error("internal error: rule too long to format\n"); }
      strcat(b, " ");
      strcat(b, ps);
      bl-=l+1;
    }
  return b;
#undef BSIZE
}

/* ------------------------------------------------------------ */
static void read_precondition_into_rule(model_pt m, rule_pt r, int n, char *s, int tf)
{
  if (1==sscanf(s, "tag[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_TAG;
      s=strchr(s, '='); s++;
      r->pc[n].u.tag=(tf && !strcmp(s, jokerstring)) ? -1 : iregister_add_name(m->tags, REGISTER_STRING(s));
    }
  else if (1==sscanf(s, "word[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_WORD;
      s=strchr(s, '='); s++;
      r->pc[n].u.word=(tf && !strcmp(s, jokerstring)) ? NULL : REGISTER_STRING(s);
    }
  else if (tf && 2==sscanf(s, "prefix[%d]=%zu", &r->pc[n].pos, &r->pc[n].u.prefix.length))
    {
      r->pc[n].type=PRE_PREFIX;
      r->pc[n].u.prefix.prefix=NULL;
    }
  else if (1==sscanf(s, "prefix[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_PREFIX;
      s=strchr(s, '='); s++;
      r->pc[n].u.prefix.prefix=REGISTER_STRING(s);
      r->pc[n].u.prefix.length=strlen(s);
    }
  else if (tf && 2==sscanf(s, "suffix[%d]=%zu", &r->pc[n].pos, &r->pc[n].u.suffix.length))
    {
      r->pc[n].type=PRE_SUFFIX;
      r->pc[n].u.suffix.suffix=NULL;
    }
  else if (1==sscanf(s, "suffix[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_SUFFIX;
      s=strchr(s, '='); s++;
      r->pc[n].u.suffix.suffix=REGISTER_STRING(s);
      r->pc[n].u.suffix.length=strlen(s);
    }
  else if (1==sscanf(s, "bos[%d]", &r->pc[n].pos))
    { r->pc[n].type=PRE_BOS; }
  else if (1==sscanf(s, "eos[%d]", &r->pc[n].pos))
    { r->pc[n].type=PRE_EOS; }
  else if (1==sscanf(s, "digit[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_DIGIT;
      s=strchr(s, '='); s++;
      if (!strcmp(s, "no")) { r->pc[n].u.digit=PRE_NO; }
      else if (!strcmp(s, "some")) { r->pc[n].u.digit=PRE_SOME; }
      else if (!strcmp(s, "all")) { r->pc[n].u.digit=PRE_ALL; }
      else if (tf && !strcmp(s, "*")) { r->pc[n].u.digit=PRE_ANY; }
      else { error("can't parse DIGIT subtype: \"%s\"\n", s); }
    }
  else if (1==sscanf(s, "cap[%d]=", &r->pc[n].pos))
    {
      r->pc[n].type=PRE_CAP;
      s=strchr(s, '='); s++;
      if (!strcmp(s, "no")) { r->pc[n].u.cap=PRE_NO; }
      else if (!strcmp(s, "some")) { r->pc[n].u.cap=PRE_SOME; }
      else if (!strcmp(s, "all")) { r->pc[n].u.cap=PRE_ALL; }
      else if (tf && !strcmp(s, "*")) { r->pc[n].u.cap=PRE_ANY; }
      else { error("can't parse CAP subtype: \"%s\"\n", s); }
    }
  else if (!strcmp(s, "ldc"))
    { r->pc[n].type=PRE_LDC; }
  else if (1==sscanf(s, "rare[%d]", &r->pc[n].pos))
    { r->pc[n].type=PRE_RARE; }
  else
    { error("can't parse precondition \"%s\"\n", s); }
}

/* ------------------------------------------------------------ */
/* previously inlined */
static rule_pt find_rule(model_pt m, char *rs)
{ return (rule_pt)hash_get(m->rulehash, rs); }

/* ------------------------------------------------------------ */
static rule_pt register_rule(model_pt m, rule_pt r)
{
  char *rs=rule2string(m, r);
  rule_pt hr=find_rule(m, rs);

  if (!hr)
    {
      hr=new_rule(r);
      hr->string=REGISTER_STRING(rs);
      hash_put(m->rulehash, hr->string, hr);
    }
  return hr;
}

/* ------------------------------------------------------------ */
static int read_rules_file(model_pt m, char*fn)
{
  FILE *f=fopen(fn, "r");
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;
  int lno, cno;

  if (!f)
    {
	    return -1;
    }
  
  lno=0;
  cno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      rule_t rt;

      s=strtok(s, " \t");
      if (!s) { continue; }      
      if (s[0]=='#' && s[1]=='#') { cno++; continue; }      
      rt.tag=iregister_add_name(m->tags, s);
      for (rt.nop=0, s=strtok(NULL, " \t");
	   s && rt.nop<MAX_NO_PC;
	   rt.nop++, s=strtok(NULL, " \t"))
      { read_precondition_into_rule(m, &rt, rt.nop, s, 0); }
      if (s) { report(0, "rule too long (%s:%d)\n", fn, lno); }      
      array_add(m->rules, (void *)new_rule(&rt));
    }
  fclose(f);
  if(buf) {
	  free(buf);
	  buf = NULL;
  }
  lno=array_count(m->rules);
  report(2, "read %d rule(s) (%d comment(s)) from \"%s\"\n", lno, cno, fn);
  return lno;
}

/* ------------------------------------------------------------ */
static void read_dictionary_file(const char*fn, model_pt m)
{
  FILE *f=try_to_open(fn, "r");
  int *tagcount;
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;
  int cno, lno, i, mft, mftc, not;
  int c[4]={ 0, 0, 0, 0 };

  /* first pass through lexicon: find tags */
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      for (s=strtok(s, " \t"), s=strtok(NULL, " \t");
           s;
           s=strtok(NULL, " \t"), s=strtok(NULL, " \t"))
        { (void)iregister_add_name(m->tags, s); }
    }
  not=iregister_get_length(m->tags);
  tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(tagcount, 0, not*sizeof(int));

  /* rewind file */
  if (fseek(f, 0, SEEK_SET)) { error("can't rewind file \"%s\"\n", fn); }

  /* second pass through file: collect details */
  
  lno=0;
  cno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      word_pt w;
      ptrdiff_t bcnt, btag;
      char *t;
      
      s=strtok(s, " \t");
      if (!s) { continue; }
      if (s[0]=='#' && s[1]=='#') { cno++; continue; }
      s=REGISTER_STRING(s);
      w=new_word(s, 0, not);
      bcnt=btag=-1;
      for (t=strtok(NULL, " \t"); t;  t=strtok(NULL, " \t"))
	{
	  ptrdiff_t cnt, ti=iregister_add_name(m->tags, t);
	  
	  t=strtok(NULL, " \t");
	  if (!t || 1!=sscanf(t, "%td", &cnt))
	    { report(1, "can't find tag count (%s:%d)\n", fn, lno); continue; }
	  if (cnt>bcnt) { bcnt=cnt; btag=ti; }
	  w->tagcount[ti]=cnt;
	  w->count+=cnt;
	}
      if (btag<0) 
      { report(0, "invalid lexicon entry (%s:%d)\n", fn, lno); continue; }
      c[0]++; c[1]+=w->count;
      w->defaulttag=btag;
      if (hash_put(m->dictionary, s, (void *)w))
	{ report(0, "duplicate lexicon entry \"%s\" (%s:%d)\n", s, fn, lno); }

      /* */
      if (m->rwt>0 && w->count<=m->rwt)
	{	  
	  /* don't consider for default tag if word is not rare */
	  for (i=0; i<not; i++)
	    {
	      int tc=w->tagcount[i];
	      if (tc==0) { continue; }
	      tagcount[i]+=tc;
	    }
	}
      else { c[2]++; c[3]+=w->count; } 	
    }
  fclose(f);
  if(buf) {
	  free(buf);
	  buf = NULL;
  }

  report(2, "read lexicon file \"%s\" (%d comment(s)):\n", fn, cno);
  report(2, "      rare: %10d type(s) %10d tokens\n", c[0]-c[2], c[1]-c[3]);
  report(2, "  frequent: %10d type(s) %10d tokens\n", c[2], c[3]);
  report(2, "            ------------------------------------\n");
  report(2, "            %10d type(s) %10d tokens\n", c[0], c[1]);

  /* find default tag */
  mft=mftc=-1;
  for (i=0; i<not; i++)
    { int tc=tagcount[i]; if (tc>mftc) { mftc=tc; mft=i; } }
  mem_free(tagcount);
  if (mft < 0 || mft >= (int) iregister_get_length(m->tags))
    { report(0, "warning: no tag in lexicon, using zero for mft\n"); mft=0; }
  else
    { report(2, "most frequent rare tag \"%s\" (%d occurences)\n", iregister_get_name(m->tags, mft), mftc); }
  m->defaulttag=mft;
}

/* ------------------------------------------------------------ */
static array_pt read_cooked_file(model_pt m, char *name)
{
  char *fn= name ? name : "STDIN";
  FILE *f= name ? try_to_open(name, "r") : stdin;
  array_pt sts=array_new(5000);
  int lno, sc=0;
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;

  lno = 0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      array_pt st=array_new(8);
      char *w, *t;

      for (w=strtok(s, " \t"); w; w=strtok(NULL, " \t"))
	{
	  sample_pt sp;
	  w=REGISTER_STRING(w);
	  t=strtok(NULL, " \t");
	  if (!t)
	    { report(0, "can't read tag (%s:%d)\n", fn, lno); continue; }
	  sp=new_sample();
	  sp->pos=array_count(st);
	  sp->word=w;
	  sp->tag=sp->tmptag=-1;
	  sp->reference=iregister_add_name(m->tags, t);
	  array_add(st, sp);
	  sc++;
	}
      array_add(sts, st);
    }
  fclose(f);
  if(buf) {
	  free(buf);
	  buf = NULL;
  }
  report(2, "read %d sentences with %d word/tag pairs from \"%s\"\n",
	 array_count(sts), sc, fn);
  return sts;
}

/* ------------------------------------------------------------ */
static void assign_lexical_tag(void *p, void *data, void *globaldata)
{ 
  sample_pt sp=(sample_pt)p;
  model_pt m=(model_pt)data;
  globals_pt g=(globals_pt)globaldata;
  int mft;

  if (is_rare(m, sp->word)) { mft=m->defaulttag; }
  else
    {
      word_pt w=get_word(m, sp->word);
      mft= w ? w->defaulttag : m->defaulttag;
    }
  
  sp->tag=sp->tmptag=mft;
  if (sp->tag==sp->reference) { g->pos++; } else { g->neg++; }
}

/* ------------------------------------------------------------ */
static void assign_lexical_tags(void *p, void *data, void *globaldata)
{ array_map2((array_pt)p, assign_lexical_tag, data, globaldata); }

/* ------------------------------------------------------------ */
static int cap_subtype(char *s)
{
  if (get_first_uppercase(s))
    {
      if (uppercase_prefix_length(s)==strlen(s)) { return PRE_ALL; }
      else { return PRE_SOME; }
    }
  else { return PRE_NO; }
}

/* ------------------------------------------------------------ */
static int digit_subtype(char *s)
{
  if (strpbrk(s, "0123456789"))
    {
      if (strspn(s, "0123456789")==strlen(s)) { return PRE_ALL; }
      else { return PRE_SOME; }
    }
  else { return PRE_NO; }
}

/* ------------------------------------------------------------ */
static int precondition_satisfied(model_pt m, array_pt sps, int pos, rule_pt r, int pcn)
{
  precondition_pt pc=&r->pc[pcn];
  int rp = pos+pc->pos;
  sample_pt sp= (rp>=0 && rp<(ssize_t)array_count(sps)) ? (sample_pt)array_get(sps, rp) : NULL;

  switch (pc->type)
    {
    case PRE_TAG:
      return sp && sp->tag==pc->u.tag;
    case PRE_WORD:
      /* use strcmp because during tagging words are not registered */
      return sp && !strcmp(sp->word, pc->u.word);
    case PRE_PREFIX:
      return sp && strstr(sp->word, pc->u.prefix.prefix)==sp->word;
    case PRE_SUFFIX:
      return sp && (common_suffix_length(sp->word, pc->u.suffix.suffix) == pc->u.suffix.length);
    case PRE_BOS:
      return rp==-1;
    case PRE_EOS:
      return rp==(ssize_t)array_count(sps);
    case PRE_DIGIT:
      return sp && pc->u.digit==digit_subtype(sp->word);
    case PRE_CAP:
      return sp && pc->u.cap==cap_subtype(sp->word);
    case PRE_LDC:
      return 0;
    case PRE_RARE:
      return sp && is_rare(m, sp->word);
    default:
      error("type of precondition %d not known", pc->type);      
    }
  return 0;
}

/* ------------------------------------------------------------ */
static int rule_matches_sample(model_pt m, array_pt sps, int pos, rule_pt r)
{
  sample_pt sp=(sample_pt)array_get(sps, pos);
  word_pt w=get_word(m, sp->word);
  int i;

  /* Only allow lexical tags for frequent words. */
  if (!is_rare(m, sp->word) && (0==w->tagcount[r->tag])) { return 0; }
  for (i=0; i<r->nop; i++)
    {
#if 0
	if (!strcmp(sp->word, "a") && !strcmp(rule2string(m, r), "NE rare[0] suffix[0]=a"))
	{
	  int x=w?w->tagcount[r->tag]:0;
	  report(-1, "RMS: %d ->%s%s[%d]/%s<- %d\n", i,
		 sp->word, is_rare(m, sp->word)?"*":"", x, iregister_get_name(m->tags, sp->tmptag),
		 precondition_satisfied(m, sps, pos, r, i));
	}
#endif
      if (!precondition_satisfied(m, sps, pos, r, i)) { return 0; }
    }

  return 1;
}

/* ------------------------------------------------------------ */
static void append_rule_to_rule_file(model_pt m, char*fn, rule_pt r)
{
  FILE *f=fopen(fn, "a");
  if (!f)
    { error("can't open file \"%s\" in append mode: %s\n", fn, strerror(errno)); }
  
  fprintf(f, "%s\n", r->string);
  fclose(f);
}

/* ------------------------------------------------------------ */

/* by Tiago Tresoldi -- this function is currently not called; commenting it out */
/*static void set_tmptag_to_tag(void *a)
{ sample_pt sp=(sample_pt)a; sp->tmptag=sp->tag; }
*/

/* ------------------------------------------------------------ */
static void set_tag_to_tmptag(void *a)
{ sample_pt sp=(sample_pt)a; sp->tag=sp->tmptag; }

/* ------------------------------------------------------------ */
static int
apply_rule(model_pt m, array_pt sts, rule_pt r, int countonly)
{
  int g=0, b=0, delta=0;
  size_t i, j;

  for (i=0; i<array_count(sts); i++)
    {
      array_pt sps=(array_pt)array_get(sts, i);
      for (j=0; j<array_count(sps); j++)
	{
	  sample_pt sp=(sample_pt)array_get(sps, j);
	  sp->tmptag=sp->tag;
	  /* applicable? */
	  if (!rule_matches_sample(m, sps, j, r)) { continue; }
	  /* if nothing changes jump out */	  
	  if (r->tag==sp->tag) { continue; }
	  if (r->tag==sp->reference)
#if 1
	    { g++; delta++; }
#else
	  {
	    sample_pt spm1= j>0 ? (sample_pt)array_get(sps, j-1) : NULL;
	    sample_pt spp1= j+1<array_count(sps) ? (sample_pt)array_get(sps, j+1) : NULL;
	    char *ttm1= spm1 ? iregister_get_name(m->tags, spm1->tag) : "NONE";
	    char *ttp1= spp1 ? iregister_get_name(m->tags, spp1->tag) : "NONE";
	    g++; delta++;
	    if (!strcmp(rule2string(m, r), "NE rare[0] suffix[0]=a"))
	      {
		word_pt w=get_word(m, sp->word);
		int x=w?w->tagcount[r->tag]:0;
		report(-1, "POS1: %d %s/%s ->%s%s[%d]/%s<- %s/%s\n", j,
		       spm1?spm1->word:"B", ttm1,
		       sp->word, is_rare(m, sp->word)?"*":"", x, iregister_get_name(m->tags, sp->tmptag),
		       spp1?spp1->word:"B", ttp1);
	      }
	  }	  
#endif
	  else if (sp->tag==sp->reference)
#if 1
	    { b++; delta--; }
#else
	  {
	    sample_pt spm1= j>0 ? (sample_pt)array_get(sps, j-1) : NULL;
	    sample_pt spp1= j+1<array_count(sps) ? (sample_pt)array_get(sps, j+1) : NULL;
	    char *ttm1= spm1 ? iregister_get_name(m->tags, spm1->tag) : "NONE";
	    char *ttp1= spp1 ? iregister_get_name(m->tags, spp1->tag) : "NONE";
	    b++; delta--;
	    if (!strcmp(rule2string(m, r), "NE rare[0] suffix[0]=a"))
	      {
		word_pt w=get_word(m, sp->word);
		int x=w?w->tagcount[r->tag]:0;
		report(-1, "NEG1: %d %s/%s ->%s%s[%d]/%s<- %s/%s\n", j,
		       spm1?spm1->word:"B", ttm1,
		       sp->word, is_rare(m, sp->word)?"*":"", x, iregister_get_name(m->tags, sp->tmptag),
		       spp1?spp1->word:"B", ttp1);
	      }
	  }
#endif
	  sp->tmptag=r->tag;
	}
      if (!countonly) { array_map(sps, set_tag_to_tmptag); }
    }
  report(-1, "rule %s %d - %d == %d\n", rule2string(m, r), g, b, delta);
  return delta;
}

/* ------------------------------------------------------------ */
static void apply_rule_to_sts(void *p, void *d1, void *d2, void *d3)
{
  rule_pt r=(rule_pt)p;
  array_pt sts=(array_pt)d1;
  model_pt m=(model_pt)d2;
  globals_pt g=(globals_pt)d3;
  int delta=apply_rule(m, sts, r, 0);  
  g->pos+=delta;
  g->neg-=delta;
}

/* ------------------------------------------------------------ */
static void read_template_file(model_pt m, char* fn)
{
  FILE *f=try_to_open(fn, "r");
  int cno, lno;

  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;

  cno = 0;
  lno = 0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      rule_t rt;
      rule_pt r;
      
      s=strtok(s, " \t");
      if (!s) { continue; }
      if (s[0]=='#' || s[1]=='#') { cno++; continue; }
      rt.tag= strcmp(s, jokerstring) ? iregister_add_name(m->tags, s) : -1;
      for (rt.nop=0, s=strtok(NULL, " \t");
	   rt.nop<MAX_NO_PC && s;
	   rt.nop++, s=strtok(NULL, " \t"))
      { read_precondition_into_rule(m, &rt, rt.nop, s, 1); }
      if (s) { error("template too long (%s:%d)\n", fn, lno); }      
      r=new_rule(&rt);
      r->string=REGISTER_STRING(rule2string(m, r));
      array_add(m->templates, (void *)r); 
    }
  fclose(f);
  if(buf) {
	  free(buf);
	  buf = NULL;
  }
  lno=array_count(m->templates);
  report(2, "read %d template(s) (%d comment(s)) from \"%s\"\n", lno, cno, fn);
}

/* ------------------------------------------------------------ */
static void free_preload_sentence(void *p)
{
  array_pt sps=(array_pt)p;
  array_map(sps, (void (*)(void *))free_sample);
  array_free(sps);
}

/* ------------------------------------------------------------ */
static void preload_file(model_pt m, globals_pt g, char *fn, array_pt sts)
{
  size_t i, j;
  array_pt pls=read_cooked_file(m, fn);

  if (array_count(pls)!=array_count(sts))
    { error("sentence file and preload file have different sizes\n"); }
  for (i=0; i<array_count(sts); i++)
    {
      array_pt ssps=(array_pt)array_get(sts, i);
      array_pt psps=(array_pt)array_get(pls, i);
      if (array_count(psps)!=array_count(ssps))
	{ error("sentence file and preload file have different sizes for sentence %d\n", i); }
      for (j=0; j<array_count(ssps); j++)
	{
	  sample_pt ssp=(sample_pt)array_get(ssps, j);
	  sample_pt psp=(sample_pt)array_get(psps, j);
	  if (ssp->word!=psp->word)
	    { error("word mismatch %s/%s in %d sentence/preload file\n", ssp->word, psp->word, i); }
	  ssp->tag=ssp->tmptag=psp->reference;
	  if (ssp->tag==ssp->reference) { g->pos++; } else { g->neg++; }
	}
    }
  array_map(pls, free_preload_sentence);
  array_free(pls);
}

/* ------------------------------------------------------------ */
static int
precondition_from_template(model_pt m, array_pt sps, int pos, rule_pt t, rule_pt r)
{
  precondition_pt rpc=&r->pc[r->nop];
  precondition_pt tpc=&t->pc[r->nop];
  int rp=pos+tpc->pos;
  sample_pt sp = NULL;

  if (tpc->type!=PRE_BOS && tpc->type!=PRE_EOS)
    {
      if (rp < 0 || rp >= (ssize_t)array_count(sps)) { return 0; }
      sp=(sample_pt)array_get(sps, rp);
    }
  rpc->type=tpc->type;
  rpc->pos=tpc->pos;
  switch (rpc->type)
    {
    case PRE_TAG:
      if (tpc->u.tag>=0 && tpc->u.tag!=sp->reference) { return 0; }
      rpc->u.tag=sp->tag;
      return 1;
    case PRE_WORD:
      if (is_rare(m, sp->word) || (tpc->u.word && tpc->u.word!=sp->word)) { return 0; }
      rpc->u.word=sp->word;
      return 1;
    case PRE_PREFIX:
      if (tpc->u.prefix.prefix)
	{
	  if (strstr(sp->word, tpc->u.prefix.prefix)==sp->word) { return 0; }
	  rpc->u.prefix.prefix=tpc->u.prefix.prefix;
	}
      else
	{
	  char* tmp = NULL;
	  size_t tmp_n = 0;
	  char *s=substr(sp->word, 0, tpc->u.prefix.length, &tmp, &tmp_n);
	  if (strlen(s) != tpc->u.prefix.length) {
		  if(tmp != NULL) {
			  free(tmp);
			  tmp = NULL;
		  }
		  return 0;
	  }
	  rpc->u.prefix.prefix=REGISTER_STRING(s);
	  if(tmp != NULL) {
		  free(tmp);
		  tmp = NULL;
	  }
	}
      rpc->u.prefix.length=tpc->u.prefix.length;
      return 1;
    case PRE_SUFFIX:
      if (tpc->u.suffix.suffix)
	{
	  if (common_suffix_length(sp->word, tpc->u.suffix.suffix) != tpc->u.suffix.length)
	    { return 0; }
	  rpc->u.suffix.suffix=tpc->u.suffix.suffix;
	}
      else
	{
	  char* tmp = NULL;
	  size_t tmp_n = 0;
	  char *s=substr(sp->word, strlen(sp->word)-1, -tpc->u.suffix.length, &tmp, &tmp_n);
	  if (strlen(s) != tpc->u.suffix.length) {
		  if(tmp != NULL) {
			  free(tmp);
			  tmp = NULL;
		  }
		  return 0;
	  }
	  rpc->u.suffix.suffix=REGISTER_STRING(s);
	  if(tmp != NULL) {
		  free(tmp);
		  tmp = NULL;
	  }
	}
      rpc->u.suffix.length=tpc->u.suffix.length;
      return 1;
    case PRE_BOS:
      return rp==-1;
    case PRE_EOS:
      return rp==(ssize_t)array_count(sps);
    case PRE_DIGIT:
      rpc->u.digit=digit_subtype(sp->word);
      if (tpc->u.digit!=PRE_ANY && tpc->u.digit!=rpc->u.digit) { return 0; }
      return 1;
    case PRE_CAP:
      rpc->u.cap=cap_subtype(sp->word);
      if (tpc->u.cap!=PRE_ANY && tpc->u.cap!=rpc->u.cap) { return 0; }
      return 1;
    case PRE_LDC:
      return 0;
    case PRE_RARE:
      return is_rare(m, sp->word);
    default:
      error("type of precondition %d not known", tpc->type);      
    }
  return 0;
}

/* ------------------------------------------------------------ */
static rule_pt
make_rule(model_pt m, array_pt sps, int pos, rule_pt t)
{
  static rule_t rt;
  sample_pt sp=(sample_pt)array_get(sps, pos);

  rt.tag=sp->reference;
  rt.string=NULL;
  rt.lic=-1;
  
  for (rt.nop=0; rt.nop<t->nop; rt.nop++)
    { if (!precondition_from_template(m, sps, pos, t, &rt)) { return NULL; } }
  return (rule_pt)&rt;
}

/* ------------------------------------------------------------ */
static void
make_rules(model_pt m, array_pt sps, int pos, array_pt rs, int goodonly)
{
  int not=iregister_get_length(m->tags);
  size_t i;
  sample_pt sp=(sample_pt)array_get(sps, pos);
  word_pt w=get_word(m, sp->word);
  int israre=is_rare(m, sp->word);

  for (i=0; i<array_count(m->templates); i++)
    {
      rule_pt r, t=(rule_pt)array_get(m->templates, i);

      /* If template specifies a tag, it must match the reference. */
      if (goodonly && t->tag>=0 && t->tag!=sp->reference) { continue; }

      r=make_rule(m, sps, pos, t);
      if (!r) { continue; }
      /* single correcting rule */
      if (goodonly)
	{ if (israre || w->tagcount[r->tag]>0) { array_add(rs, new_rule(r)); } }
      else
	{
	  /* template with specific tag: maybe worsening rule */
	  if (t->tag>=0)
	    {
	      r->tag=t->tag;
	      if (israre || w->tagcount[r->tag]>0)
		{ array_add(rs, new_rule(r)); }
	    }
	  /* template without specific tag: all target tags */
	  else
	    {
	      for (r->tag=0; r->tag<not; r->tag++)
		{ if (israre || w->tagcount[r->tag]>0) { array_add(rs, new_rule(r)); } }
	    }
	}
    }
}

/* ------------------------------------------------------------ */
/* previously inlined */
static int rule_score(rule_pt r)
{ return !r ? -1000 : r->delta*10 - r->nop; }

/* ------------------------------------------------------------ */
static rule_pt find_best_rule(model_pt m)
{
  rule_pt r, br=NULL;
  int brs=rule_score(br);
  hash_iterator_pt hi=hash_iterator_new(m->rulehash);

  for (r=(rule_pt)hash_iterator_next_value(hi);
       r;
       r=(rule_pt)hash_iterator_next_value(hi))
    { int rs=rule_score(r); if (rs>brs) { br=r; brs=rs; } }
  hash_iterator_delete(hi);
  return br;
}

/* ------------------------------------------------------------ */
static void register_correcting_rules(void *a, void *b)
{
  array_pt sps=(array_pt)a, rs=array_new(8);
  model_pt m=(model_pt)b;
  size_t i;
  
  for (i=0; i<array_count(sps); i++)
    {
      sample_pt sp=(sample_pt)array_get(sps, i);
      int nor, l;

      if (sp->reference==sp->tag) { continue; }
      make_rules(m, sps, i, rs, 1);
      nor=array_count(rs);
      for (l=0; l<nor; l++)
	{
	  rule_pt r=(rule_pt)array_get(rs, l);
	  (void)register_rule(m, r);
	  /* 	      report(-1, "%s\n", rule2string(m, r)); */
	  free_rule(r);
	  /* print_rule(hr, m); */
	}
      array_clear(rs);
    }
  array_free(rs);
}

/* ------------------------------------------------------------ */
static void free_rule_key_value(void *k, void *v)
{ free_rule((rule_pt)v); }

/* ------------------------------------------------------------ */
static void make_deltas(void *a, void *b)
{
  array_pt sps=(array_pt)a, rs=array_new(8);
  model_pt m=(model_pt)b;
  size_t i;

  for (i=0; i<array_count(sps); i++)
    {
      sample_pt sp=(sample_pt)array_get(sps, i);
      int nor, l;
      
      make_rules(m, sps, i, rs, 0);
      nor=array_count(rs);
      for (l=0; l<nor; l++)
	{
	  rule_pt r=(rule_pt)array_get(rs, l);
	  char *rs=rule2string(m, r);
	  rule_pt hr=find_rule(m, rs);
	  
	  free_rule(r);
	  if (!hr) { continue; }
	  if (hr->tag==sp->tag) { continue; }
	  if (hr->tag==sp->reference)
#if 1
	    { hr->good++; hr->delta++; }
#else
	  {
	    sample_pt spm1= i>0 ? (sample_pt)array_get(sps, i-1) : NULL;
	    sample_pt spp1= i+1<array_count(sps) ? (sample_pt)array_get(sps, i+1) : NULL;
	    char *ttm1= spm1 ? iregister_get_name(m->tags, spm1->tag) : "NONE";
	    char *ttp1= spp1 ? iregister_get_name(m->tags, spp1->tag) : "NONE";
	    hr->good++; hr->delta++;
	    if (!strcmp(rule2string(m, hr), "NE rare[0] suffix[0]=a"))
	      {
		word_pt w=get_word(m, sp->word);
		int x=w?w->tagcount[hr->tag]:0;
		report(-1, "POS2: %d %s/%s ->%s%s[%d]/%s<- %s/%s\n", i,
		       spm1?spm1->word:"B", ttm1,
		       sp->word, is_rare(m, sp->word)?"*":"", x, iregister_get_name(m->tags, sp->tmptag),
		       spp1?spp1->word:"B", ttp1);
	      }
	  }
#endif	  
	  else if (sp->tag==sp->reference)
#if 1
	    { hr->bad++; hr->delta--; }
#else
	  {
	    sample_pt spm1= i>0 ? (sample_pt)array_get(sps, i-1) : NULL;
	    sample_pt spp1= i+1<array_count(sps) ? (sample_pt)array_get(sps, i+1) : NULL;
	    char *ttm1= spm1 ? iregister_get_name(m->tags, spm1->tag) : "NONE";
	    char *ttp1= spp1 ? iregister_get_name(m->tags, spp1->tag) : "NONE";
	    hr->bad++; hr->delta--;
	    if (!strcmp(rule2string(m, hr), "NE rare[0] suffix[0]=a"))
	      {
		word_pt w=get_word(m, sp->word);
		int x=w?w->tagcount[hr->tag]:0;
		report(-1, "NEG2: %d %s/%s ->%s%s[%d]/%s<- %s/%s\n", i,
		       spm1?spm1->word:"B", ttm1,
		       sp->word, is_rare(m, sp->word)?"*":"", x, iregister_get_name(m->tags, sp->tmptag),
		       spp1?spp1->word:"B", ttp1);
	      }
	  }
#endif
	}
      array_clear(rs);
    }
  array_free(rs);
}

/* ------------------------------------------------------------ */

/* md: minimum improvement per iteration */
/* mi: maximum number of iterations */
static void training(model_pt m, globals_pt g, int mi, int md)
{
  array_pt sts=read_cooked_file(m, g->ipf), rs=array_new(128);
  rule_pt br;
  int i;
  
  read_template_file(m, g->tf);

  /* either pre-tagged file or apply lexical guess */
  if (g->plf) { preload_file(m, g, g->plf, sts); report(2, "after preload:"); }
  else
    {
      array_map2(sts, assign_lexical_tags, m, g);
      if (array_count(m->rules)==0)
	{
	  /* enforce the default rare tag with a rule*/
	  rule_t rt;
	  rt.tag=m->defaulttag;
	  rt.nop=1;
	  rt.pc[0].type=PRE_RARE;
	  rt.pc[0].pos=0;
	  rt.string=rule2string(m, &rt);
	  append_rule_to_rule_file(m, g->rf, &rt);
	  report(2, "adding lexical default rule \"%s\"\n", rt.string);
	}
      report(2, "after lexicon check:");
    }
  report(-2, " %dp + %dn==%d accuracy %7.3f%%\n", g->pos, g->neg, g->pos+g->neg,
	 g->pos+g->neg==0 ? 0.0 : 100.0*g->pos/(g->pos+g->neg));
  
  /* apply rules if any */
  if (array_count(m->rules)>0)
    {
	    array_map3(m->rules, apply_rule_to_sts, sts, m, g);
      report(2, "after rule application: %dp + %dn==%d accuracy %7.3f%%\n",
	     g->pos, g->neg, g->pos+g->neg,
	     g->pos+g->neg==0 ? 0.0 : 100.0*g->pos/(g->pos+g->neg));
    }

  /* find all correcting rules, add deltas */
  array_map1(sts, register_correcting_rules, m);
  report(1, "initially generated %d rules\n", hash_size(m->rulehash));
  array_map1(sts, make_deltas, m);

  /* get best rule & update loop */
  for (i=1, br=find_best_rule(m);
       br && br->delta >= md && (mi < 0 || i <= mi);
       i++, br=find_best_rule(m))
    {
      int delta=apply_rule(m, sts, br, 0);
      
      report(1, "best rule is %s delta %d good %d - bad %d == %d\n",
	     br->string, br->delta, br->good, br->bad, br->good-br->bad);
      append_rule_to_rule_file(m, g->rf, br);
      if (br->delta!=delta)
	{ error("ERROR: internal delta mismatch %d %d\n", br->delta, delta); }
      g->pos+=delta; g->neg-=delta;
      report(2, "iteration %d: %dp + %dn==%d delta %d accuracy %7.3f%%\n",
	     i, g->pos, g->neg, g->pos+g->neg, delta, 100.0*g->pos/(g->pos+g->neg));
      hash_map(m->rulehash, free_rule_key_value);
      hash_clear(m->rulehash);
      array_map1(sts, register_correcting_rules, m);
      array_map1(sts, make_deltas, m);
    }
  array_free(rs);
}

/* ------------------------------------------------------------ */
static void tagging(model_pt m, globals_pt g)
{
  FILE *f= g->ipf ? try_to_open(g->ipf, "r") : stdin;  
  array_pt pool=array_new(128), sps=array_new(128);
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;
  int lno;
  
  lno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      char *t;
      size_t i;
      array_clear(sps);
      for (i=0, t=strtok(s, " \t"); t; i++, t=strtok(NULL, " \t"))
	{
	  sample_pt sp;

	  /* preallocate a pool of samples, reuse later */
	  if (i>=array_count(pool))
	    {
	      int j, asp=array_size(pool);
	      for (j=0; j<asp; j++)
		{ array_add(pool, (void *)new_sample()); }
	    }
	  sp=(sample_pt)array_get(pool, i);
	  sp->word=t;
	  if (g->rawinput) { assign_lexical_tag((void *)sp, (void *)m, (void *)g); }
	  else
	    {
	      t=strtok(NULL, " \t");
	      if (!t)
		{ error("can't find tag #%d in cooked input (%s:%d)\n", i, g->ipf ? g->ipf : "STDIN", lno); }
	      sp->tag=iregister_add_name(m->tags, t);
	    }
	  sp->tmptag=sp->tag;
	  array_add(sps, sp);
	}
      /* now that we have the sentence, apply rules */
      for (i=0; i<array_count(m->rules); i++)
	{
	  rule_pt r=(rule_pt)array_get(m->rules, i);
	  size_t j;
	  for (j=0; j<array_count(sps); j++)
	    {
	      sample_pt sp=(sample_pt)array_get(sps, j);
	      if (!rule_matches_sample(m, sps, j, r)) { continue; }
/* 	      sp->tag=sp->tmptag=r->tag; */
	      sp->tmptag=r->tag;
	    }
	  array_map(sps, set_tag_to_tmptag);
	}
      /* print cooked sentence */
      for (i=0; i<array_count(sps); i++)
	{
	  sample_pt sp=(sample_pt)array_get(sps, i);
	  if (i>0) { fprintf(stdout, " "); }
	  fprintf(stdout, "%s %s", sp->word, iregister_get_name(m->tags, sp->tmptag));
	}
      fprintf(stdout, "\n");
    }
  array_free(sps);
  array_map(pool, (void (*)(void *))free_sample);
  array_free(pool);
  if (f!=stdin) { fclose(f); }
  if(buf) {
	  free(buf);
	  buf = NULL;
  }
}

/* ------------------------------------------------------------ */
static void unknown_vs_known2(void *p, void *d1, void *d2)
{
  sample_pt sp=(sample_pt)p;
  int *c=(int *)d1;
  model_pt m=(model_pt)d2;
  word_pt w=get_word(m, sp->word);

  if (sp->tag==sp->reference)
    { if (w) { c[0]++; } else { c[1]++; } }
  else
    { if (w) { c[2]++; } else { c[3]++; } }
}  

/* ------------------------------------------------------------ */
static void unknown_vs_known1(void *p, void *d1, void *d2)
{ array_map2((array_pt)p, unknown_vs_known2, d1, d2); }

/* ------------------------------------------------------------ */
static void testing(model_pt m, globals_pt g)
{
  size_t i;
  int c[4]={0, 0, 0, 0};
  array_pt sts=read_cooked_file(m, g->ipf);

  /* either pre-tagged file or apply lexical guess */
  if (g->plf) { preload_file(m, g, g->plf, sts); report(2, "after preload:"); }
  else
  { array_map2(sts, assign_lexical_tags, m, g); report(2, "after lexicon check:"); }
  report(-2, " %dp + %dn==%d accuracy %7.3f%%\n", g->pos, g->neg, g->pos+g->neg,
	 g->pos+g->neg==0 ? 0.0 : 100.0*g->pos/(g->pos+g->neg));

  /*
    array_map1(sts, assign_lexical_tags, m);
    report(2, "after lexicon check: %dp + %dn==%d accuracy %7.3f%%\n",
    g->pos, g->neg, g->pos+g->neg, 100.0*g->pos/(g->pos+g->neg));
  */
  fprintf(stdout, "%5d %d %d %7.3f %d %d %7.3f %d %d %7.3f\n", 0,
	  g->pos, g->neg, g->pos+g->neg==0 ? 0.0 : 100.0*g->pos/(g->pos+g->neg),
	  c[0], c[2], c[0]+c[2]==0 ? 0.0 : 100.0*c[0]/(c[0]+c[2]),
	  c[1], c[3], c[1]+c[3]==0 ? 0.0 : 100.0*c[1]/(c[1]+c[3]) );

  for (i=0; i<array_count(m->rules); i++)
    {
      rule_pt r=(rule_pt)array_get(m->rules, i);
      int c[4]={0, 0, 0, 0};
      apply_rule_to_sts(r, sts, m, g);
      array_map2(sts, unknown_vs_known1, c, m);
      report(2, "after rule %d: %dp + %dn==%d accuracy %7.3f%% ",
	     i+1, g->pos, g->neg, g->pos+g->neg, 100.0*g->pos/(g->pos+g->neg));
      report(-2, "known %dp %dn %7.3f%% unknown %dp %dn %7.3f%%\n",
	     c[0], c[2], 100.0*c[0]/(c[0]+c[2]), c[1], c[3], 100.0*c[1]/(c[1]+c[3]));
      fprintf(stdout, "%5lu %d %d %7.3f %d %d %7.3f %d %d %7.3f\n", (unsigned long)i+1,
	      g->pos, g->neg, g->pos+g->neg==0 ? 0.0 : 100.0*g->pos/(g->pos+g->neg),
	      c[0], c[2], c[0]+c[2]==0 ? 0.0 : 100.0*c[0]/(c[0]+c[2]),
	      c[1], c[3], c[1]+c[3]==0 ? 0.0 : 100.0*c[1]/(c[1]+c[3]) );
    }
}


static globals_pt new_globals(globals_pt old)
{
  globals_pt g=(globals_pt)mem_malloc(sizeof(globals_t));

  if (old) { memcpy(g, old, sizeof(globals_t)); return g; }

  g->rf=g->ipf=g->plf=g->tf=NULL;
  g->rawinput=0;
  g->pos=g->neg=0;
  return g;
}

/* ------------------------------------------------------------ */
int main(int argc, char **argv)
{
  model_pt model=new_model();

  int h = 0;
  unsigned long v = 1;
  long i = -1;
  long m = 1;
  long r = 0;
  int R = 0;
  char *l = NULL;
  char *p = NULL;
  char *t = NULL;
  char *u = NULL;
  enum OPTION_OPERATION_MODE o = OPTION_OPERATION_TAG;
  option_callback_data_t cd = {
    &o,
    option_operation_mode_parser,
    option_operation_mode_serializer
  };
  option_context_t options = {
	  argv[0],
	  "transformation-based part-of-speech tagger",
	  "OPTIONS rulefile [inputfile]",
	  version_copyright_banner,
	  (option_entry_t[]) {
		  { 'h', OPTION_NONE, (void*)&h, "display this help" },
		  { 'v', OPTION_UNSIGNED_LONG, (void*)&v, "verbosity level [1]" },
		  { 'i', OPTION_SIGNED_LONG, (void*)&i, "maximum number of iterations [unlimited]" },
		  { 'l', OPTION_STRING, (void*)&l, "lexicon file [none]" },
		  { 'm', OPTION_SIGNED_LONG, (void*)&m, "minimum improvement per iteration [1]" },
		  { 'r', OPTION_SIGNED_LONG, (void*)&r, "rare word threshold [0]" },
		  { 'o', OPTION_CALLBACK, (void*)&cd, "mode of operation tag, test, train [tag]" },
		  { 'p', OPTION_STRING, (void*)&p, "preload file [none]" },
		  { 'R', OPTION_NONE, (void*)&R, "assume raw format for input [cooked format]" },
		  { 't', OPTION_STRING, (void*)&t, "template file [none]" },
		  { 'u', OPTION_STRING, (void*)&u, "unknown word default tag [lexicon based]" },
		  { '\0', OPTION_NONE, NULL, NULL }
	  }
  };
  int idx = options_parse(&options, "--", argc, argv);
  if(h) {
	  options_print_usage(&options, stdout);
	  return 0;
  }

  globals_pt g=new_globals(NULL);
  g->rawinput = R;
  model->rwt = r;
  g->tf = t;
  g->plf = p;
  if (idx<argc)
  {
	  g->rf=argv[idx];
	  idx++;
  } else {
	  options_print_usage(&options, stderr);
	  error("missing rules file\n");
  }
  if (idx<argc && strcmp("-", argv[idx]))
  {
	  g->ipf=argv[idx+1];
  }
  if(o!=OPTION_OPERATION_TAG && o!=OPTION_OPERATION_TRAIN && o!=OPTION_OPERATION_TEST)
  {
	  error("invalid mode of operation \"%d\"\n", o);
  }
  if (o==OPTION_OPERATION_TRAIN && !g->tf)
  {
	  error("you must specify a template file (-t) for training\n");
  }
  if (o!=OPTION_OPERATION_TRAIN && g->tf)
  {
	  error("no template file needed in train mode\n");
  }
  if (o==OPTION_OPERATION_TAG && g->plf)
  {
	  error("no preload file needed in tag mode\n");
  }
  if (o!=OPTION_OPERATION_TAG && g->rawinput)
  {
	  error("input must be in cooked format for tag\n");
  }
  if (u)
  {
	  model->defaulttag=iregister_add_name(model->tags, u);
	  report(2, "overriding default tag for unknown word: %s\n", u);
  }

  if(v >= 1) {
	  options_print_configuration(&options, stderr);
  }

  /* TODO: find better seed */
  srand48(0);

  if (l) {
	  read_dictionary_file(l, model);
  }

  int ret = read_rules_file(model, g->rf);
  if(ret == -1) {
      if (o==OPTION_OPERATION_TRAIN)
	{ report(1, "\"%s\" seems to be a new file, good\n", g->rf); }
      else
      error("can't read rules from rule file \"%s\"\n", g->rf);
  }

  switch (o)
    {
    case OPTION_OPERATION_TAG:
      tagging(model, g); break;
    case OPTION_OPERATION_TEST:
      testing(model, g); break;
    case OPTION_OPERATION_TRAIN: 
      training(model, g, i, m); break;
    default:
      report(0, "unknown mode of operation %d\n", o);
    }

  report(1, "done\n");

  /* Free strings register */
  sregister_delete(model->strings);

  exit(0);
}

/* ------------------------------------------------------------ */
