/*
  Maximum Entropy Tagger

  Copyright (C) 2001, 2002 Ingo Schröder

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
#include "config-common.h"
#include <stddef.h> /* for ptrdiff_t and size_t. */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strtok */
#endif
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include "mem.h"
#include "array.h"
#include "hash.h"
#include "util.h"
#include "gis.h"

/* ------------------------------------------------------------ */
typedef struct sample_s
{
  int tag;
  char *word;
} sample_t;
typedef sample_t *sample_pt;

typedef enum {
  pt_word,
  pt_prefix,
  pt_suffix,
  pt_number,
  pt_uppercase,
  pt_hyphen,
  pt_tm1,
  pt_tm1tm2,
  pt_wm1,
  pt_wm2,
  pt_wp1,
  pt_wp2,
  pt_default
} ptype_e;
#define min_ptype_e ((int)pt_word)
#define max_ptype_e ((int)pt_default)
#define no_ptype_e (max_ptype_e-min_ptype_e+1)

typedef struct predinfo_s
{
  ptype_e type;                 /* type */
  char *w;                      /* parameter word */
  int t1;                       /* parameter tag1 */
  int t2;                       /* parameter tag2 */
  predicate_pt predicate;       /* gis predicate */
} predinfo_t;
typedef predinfo_t *predinfo_pt;

typedef struct predindex_s
{
  hash_pt wm2, wm1, word, wp1, wp2;
  predicate_pt wm2_null, wm1_null, wp1_null, wp2_null;
  hash_pt prefix, suffix;
  array_pt tm1, tm1tm2;
  predicate_pt number;
  predicate_pt uppercase;
  predicate_pt hyphen;
  predicate_pt def;
} predindex_t;
typedef predindex_t *predindex_pt;

typedef struct invocation_s
{
  int mode;
  char *cmd;
  char *opt;
  char *call;
} invocation_t;
typedef invocation_t *invocation_pt;

typedef struct option_s
{
  char character;
  char *usage;
} option_t;
typedef option_t *option_pt;

/* ------------------------------------------------------------ */
invocation_t ivs[]={
  { 0, "met", "b:c:d:f:hi:m:np:r:st:v:", "[OPTIONS] model [input]" },
  { 1, "met-tag", "b:d:hm:np:sv:", "[OPTIONS] model [text-file]" },
  { 2, "met-test", "b:d:hm:np:sv:", "[OPTIONS] model [corpus-file]" },
  { 3, "met-train", "d:f:hi:p:r:t:v:", "[OPTIONS] model [corpus-file]" },
  { 1, "tag", NULL, NULL },
  { 2, "test", NULL, NULL },
  { 3, "train", NULL, NULL },
  {0, NULL, NULL},
};

option_t ops[]={
  { 'b', "-b b  beam factor [1000] or n-best width [5]" },
  { 'c', "-c c  command mode [NONE]" },
  { 'd', "-d d  dictionary file [NONE]" },
  { 'f', "-f f  threshold for feature count [5]" },
  { 'h', "-h    displays help" },
  { 'i', "-i i  maximum number of iterations [100]" },
  { 'm', "-m m  probability threshold [1.0]" },
  { 'n', "-n    use n-best instead of viterbi" },
  { 'p', "-p p  priority class [19]" },
  { 'r', "-r r  rare word threshold [5]" },
  { 's', "-s    case sensitive mode for dictionary" },
  { 't', "-t t  minimum improvement between iterations [0.0]" },
  { 'v', "-v v  verbosity [1]" },
  { '\0', NULL },
};

char *banner=
"Maximum Entropy Tagger\n(c) Ingo Schröder, schroeder@informatik.uni-hamburg.de\n";

/* threshold for rare words */
unsigned int rwt=5;

/* counter of (frequent) words */
int no_w_token=0;
int no_fw_token=0;
int no_fw_types=0;

/* arrrrgh, needed for qsort */
double *dp=NULL;

/* program name */
char *cmd=NULL;

/* ------------------------------------------------------------ */
static char *register_word(char *w, size_t t, array_pt wds, array_pt wtgs, array_pt wcs, hash_pt wh)
{
  char *s=register_string(w);
  ptrdiff_t i=((ptrdiff_t)hash_get(wh, s))-1;
  array_pt tags;
  
  if (i>=0)
    {
      size_t wc=(size_t)array_get(wcs, i);
      array_set(wcs, i, (void *)(wc+1)); 
      array_add_unique((array_pt)array_get(wtgs, i), (void *)t);
      return (char*)array_get(wds, i);
    }

  i=array_add(wcs, (void *)1);
  tags=array_new(16);
  array_add_unique(tags, (void *)t);
  if (i!=array_add(wtgs, tags))
    { error("word count/tag inconsistency\n"); }
  if (i!=array_add(wds, s))
    { error("word count/string inconsistency\n"); }
  hash_put(wh, s, (void *)(i+1));
  return s;
}

/* ------------------------------------------------------------ */
static void add_feature(predinfo_pt pi, model_pt m, event_pt ev)
{
  predicate_pt pd=pi->predicate;  
  feature_pt ft=array_get(pd->features, ev->outcome);
  
  /* check whether feature exists already */
  if (!ft)
    {
      ft=new_feature(0, ev->outcome, pd);
      array_set(pd->features, ev->outcome, ft);
      m->no_fts++;
    }
  ft->count+=ev->count;
  ft->E=log((double)ft->count);

  /* put pd into ev */
  (void)array_add_unique(ev->predicates, pd);
  /* store max_pds & min_pds */
}

/* ------------------------------------------------------------ */
static predinfo_pt new_predinfo(predinfo_pt p, int no_tgs)
{
  predinfo_pt pi=(predinfo_pt)mem_malloc(sizeof(predinfo_t));

  if (p) { memcpy(pi, p, sizeof(predinfo_t)); }
  else { memset(pi, 0, sizeof(predinfo_t)); }
  pi->predicate=new_predicate(0, pi, no_tgs);
  
  return pi;
}

/* ------------------------------------------------------------ */
static void delete_predinfo(predinfo_pt p)
{
  delete_predicate(p->predicate);
  mem_free(p);
}

/* ------------------------------------------------------------ */
static void register_predinfo(predinfo_pt p, model_pt m, event_pt ev)
{
  static array_pt table[no_ptype_e]={ NULL };
  array_pt pds=m->predicates;
  array_pt ta;
  predinfo_pt pr;
  size_t i;

  if (!table[0])
    {
      for (i=min_ptype_e; i<=max_ptype_e; i++)
	{
	  size_t j;
	  table[i]=array_new(256);
	  for (j=0; j<256; j++) { array_add(table[i], array_new(32)); }	      
	}
    }

  /* poor man's hash table */
  /* first indexed on type */
  ta=table[p->type];
  /* second index depends on type */
  switch (p->type)
    {
    case pt_prefix: case pt_word:
      ta=array_get(ta, (unsigned char)p->w[0]);
      break;
    case pt_suffix:
      ta=array_get(ta, (unsigned char)p->w[strlen(p->w)-1]);
      break;
    case pt_wm1: case pt_wm2: case pt_wp1: case pt_wp2:
      ta=array_get(ta, (unsigned char)(p->w? p->w[0] : 0));
      break;
    case pt_tm1:
      ta=array_get(ta, (unsigned char)p->t1);
      break;
    case pt_tm1tm2:
      ta=array_get(ta, (unsigned char)(((p->t1+3)*(p->t2+7)%255)));
      break;
    case pt_number: case pt_uppercase: case pt_hyphen:
    case pt_default:
      ta=array_get(ta, 0);
      break;
    }

  for (i=0; i<array_count(ta); i++)
    {
      size_t l=0;
      ptype_e pt=p->type;
      pr=(predinfo_pt)array_get(ta, i);
      
      if (pt==pt_prefix)
	{ l=common_prefix_length(p->w, pr->w); }
      else if (pt==pt_suffix)
	{ l=common_suffix_length(p->w, pr->w); }

      switch (pt)
	{
	case pt_word: case pt_wm1: case pt_wm2: case pt_wp1: case pt_wp2:
	  if (p->w==pr->w) { add_feature(pr, m, ev); return; }
	  break;
	case pt_tm1:
	  if (p->t1==pr->t1) { add_feature(pr, m, ev); return; }
	  break;
	case pt_tm1tm2:
	  if (p->t1==pr->t1 && p->t2==pr->t2) { add_feature(pr, m, ev); return; }
	  break;
	case pt_number: case pt_uppercase: case pt_hyphen:
	  add_feature(pr, m, ev); return;
	  break;
	case pt_prefix:
	  if (l==strlen(p->w)) { add_feature(pr, m, ev); return; }
	  break;
	case pt_suffix:
	  if (l==strlen(p->w)) { add_feature(pr, m, ev); return; }
	  break;
	case pt_default:
	  /* nothing, just for the compiler */
	  break;
	}
    }  

  /* since we're here, p is not yet available:
     new predinfo + predicate */
  pr=new_predinfo(p, m->no_ocs);
  add_feature(pr, m, ev);  
  /* put both in global array and in index */
  array_add(pds, pr->predicate);
  array_add(ta, pr);
}

/* ------------------------------------------------------------ */
static predindex_pt new_predindex(model_pt m)
{
  predindex_pt pi=(predindex_pt)mem_malloc(sizeof(predindex_t));
  size_t not=array_count(m->outcomes);
  size_t i;
  
  memset(pi, 0, sizeof(predindex_t));
  pi->word=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->wm2=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->wm1=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->wp1=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->wp2=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->prefix=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->suffix=hash_new(1000, 0.7, hash_string_hash, hash_string_equal);
  pi->tm1=array_new_fill(not+1, NULL);
  pi->tm1tm2=array_new_fill(not+1, NULL);
  for (i=0; i<=not; i++)
    { array_set(pi->tm1tm2, i, array_new_fill(not+1, NULL)); }
  return pi;
}

/* ------------------------------------------------------------ */
static void make_event(char *w[], int t[], array_pt wcs, hash_pt wh, model_pt m, array_pt evs)
{
  predinfo_t p;
  event_pt ev=new_event(1, t[2]);
  size_t wc= (size_t) array_get(wcs, ((ptrdiff_t)hash_get(wh, w[2]))-1);

#if 0
#define W(i) w[i]?w[i]:"NULL"
  report(0, "TT %s %s %s %s %s %d %d %d %d %d\n",
	 W(0), W(1), W(2), W(3), W(4), 
	 t[0], t[1], t[2], t[3], t[4]); 
#endif
  
  array_add(evs, ev);
  if (wc>=rwt)
    { p.w=w[2]; p.type=pt_word; register_predinfo(&p, m, ev); }
  else
    {
      size_t cl=strlen(w[2]);
      size_t i;
      
      /* FIXME: bug in jmx: a X-character word cannot have prefixes
	 or suffixes of length X
      */
      for (i=1; i<5 && i<cl; i++)
	{
	  p.type=pt_prefix;
	  p.w=register_string(substr(w[2], 0, i));
	  register_predinfo(&p, m, ev);
	  p.type=pt_suffix;
	  p.w=register_string(substr(w[2], cl-1, -i));
	  register_predinfo(&p, m, ev);
	}
      p.w=w[2];
      if (strpbrk(p.w, "0123456789"))
	{ p.type=pt_number; register_predinfo(&p, m, ev); }
      if (strpbrk(p.w, "ABCDEFGHIJKLMNOPQRSTUVWXYZÖÜÄ"))
	{ p.type=pt_uppercase; register_predinfo(&p, m, ev); }
      if (strpbrk(p.w, "-"))
	{ p.type=pt_hyphen; register_predinfo(&p, m, ev); }
    }
  p.t1=t[1]; p.type=pt_tm1; register_predinfo(&p, m, ev);
  p.t2=t[0]; p.type=pt_tm1tm2; register_predinfo(&p, m, ev);

  p.w=w[0]; p.type=pt_wm2; register_predinfo(&p, m, ev);
  p.w=w[1]; p.type=pt_wm1; register_predinfo(&p, m, ev);
	  
  p.w=w[3]; p.type=pt_wp1; register_predinfo(&p, m, ev);
  p.w=w[4]; p.type=pt_wp2; register_predinfo(&p, m, ev);
}

/* ------------------------------------------------------------ */
static void sample2event(array_pt wcs, hash_pt wh,
			 char *w, int tg, model_pt m, array_pt evs)
{
  static char *wds[5]={0, 0, 0, 0, 0};
  static int tgs[5]={-1, -1, -1, -1, -1};

  do
    {
      size_t i;      
      for (i=0; i<4; i++) { wds[i]=wds[i+1]; tgs[i]=tgs[i+1]; }      
      wds[4]=w; tgs[4]=tg;
      if (wds[2]) { make_event(wds, tgs, wcs, wh, m, evs); }
    } while (!w && (wds[3] || wds[4]));
}

/* ------------------------------------------------------------ */
static sample_pt new_sample(char *w, int tg)
{
  sample_pt s=(sample_pt)mem_malloc(sizeof(sample_t));
  s->word=w;
  s->tag=tg;
  return s;  
}

/* ------------------------------------------------------------ */
static void delete_sample(sample_pt s) { mem_free(s); }

/* ------------------------------------------------------------ */
static void print_predinfo(predinfo_pt p, FILE *f, array_pt tgs)
{
#define TAG_NAME(i) ((i)<0 ? "BOUNDARY" : (char *)array_get(tgs, (i)))
#define WRD_NAME(i) ((i) ? (i) : "BOUNDARY")
  if (!f) { f=stderr; }
  switch (p->type)
    {
    case pt_word:
      fprintf(f, "curword=%s", p->w);
      break;
    case pt_wm1:
      fprintf(f, "word-1=%s", WRD_NAME(p->w));
      break;
    case pt_wm2:
      fprintf(f, "word-2=%s", WRD_NAME(p->w));
      break;
    case pt_wp1:
      fprintf(f, "word+1=%s", WRD_NAME(p->w));
      break;
    case pt_wp2:
      fprintf(f, "word+2=%s", WRD_NAME(p->w));
      break;
    case pt_tm1:
      fprintf(f, "tag-1=%s", TAG_NAME(p->t1));
      break;
    case pt_tm1tm2:
      fprintf(f, "tag-1,2=%s %s", TAG_NAME(p->t1), TAG_NAME(p->t2));
      break;
    case pt_prefix:
      fprintf(f, "prefix=%s", p->w);
      break;
    case pt_suffix:
      fprintf(f, "suffix=%s", p->w);
      break;
    case pt_number:
      fprintf(f, "numeric");
      break;
    case pt_uppercase:
      fprintf(f, "uppercase");
      break;
    case pt_hyphen:
      fprintf(f, "hyphen");
      break;
    case pt_default:
      fprintf(f, "DEFAULT");
      break;
    }
}

/* ------------------------------------------------------------ */
static int filter_and_delete_predicates(void *p)
{
  predicate_pt pd=(predicate_pt)p;
  if (pd->features) { return 0; }
  delete_predinfo(pd->data);
  return 1;
}

/* ------------------------------------------------------------ */
static int invalid_predicate(void *p)
{
  predicate_pt pd=(predicate_pt)p;
  return pd->features==NULL;
}

/* ------------------------------------------------------------ */
static void filter_predicates_from_event(void *p)
{
  event_pt ev=(event_pt)p;
  array_filter(ev->predicates, invalid_predicate); 
}

/* ------------------------------------------------------------ */
static void select_features(model_pt m, array_pt evs, size_t fmin)
{
  array_pt pds=m->predicates;
  size_t i;
  size_t s_count=0;
  size_t d_count=0;
  size_t pds_count=array_count(pds);
  
  /* discards features that are rare */
  for (i=0; i<pds_count; i++)
    {
      predicate_pt pd=(predicate_pt)array_get(pds, i);
      size_t j;
      size_t counter=0;
      
      for (j=0; j<m->no_ocs; j++)
	{
	  feature_pt ft=(feature_pt)array_get(pd->features, j);
	  if (!ft) { continue; }
	  if (ft->count<fmin && ((predinfo_pt)ft->predicate->data)->type!=pt_word)
	    {
	      array_set(pd->features, j, NULL);
	      delete_feature(ft);
	      d_count++;
	    }
	  else { counter++; s_count++; }
	}
      if (counter==0) { array_free(pd->features); pd->features=NULL; }
    }
  array_map(evs, filter_predicates_from_event);
  array_filter(pds, filter_and_delete_predicates);
#define DEBUG_SELECT_FEATURES 0
#if DEBUG_SELECT_FEATURES
  {
    FILE *f=fopen("FEATURES", "w");
    size_t i;
    for (i=0; i<array_count(pds); i++)
      {
	predicate_pt pd=(predicate_pt)array_get(pds, i);
	print_predinfo(pd->data, f, m->outcomes);
	fprintf(f, "\n");
      }
    fclose(f);
  }
#endif
  report(2, "%d features selected and %d deleted\n", s_count, d_count);
  report(2, "%d predicates left and %d deleted\n",
	 array_count(pds), pds_count-array_count(pds));
}

/* ------------------------------------------------------------ */
static void setup_cf_E(void *p, void *data)
{
  event_pt e=(event_pt)p;
  model_pt m=(model_pt)data;

  m->cf_E+=(m->max_pds - array_count(e->predicates))*e->count;
}

/* ------------------------------------------------------------ */
static void add_default_features(model_pt md, array_pt evs)
{
  array_pt pds=md->predicates;
  size_t i;
  size_t minp=array_count(pds), maxp=0;
  predinfo_pt p;
  size_t no_etoken=0;  
  
  p=new_predinfo(NULL, md->no_ocs);
  p->type=pt_default;
  array_add(pds, p->predicate);  

  /* TODO: switch to array_map */
  for (i=0; i<array_count(evs); i++)
    {
      event_pt ev=(event_pt)array_get(evs, i);
      size_t no_pds;
      
      no_etoken+=ev->count;
      add_feature(p, md, ev);
      array_trim(ev->predicates);
      no_pds=array_count(ev->predicates);
      if (minp>no_pds) { minp=no_pds; }
      if (maxp<no_pds) { maxp=no_pds; }
    }
  md->min_pds=minp;
  md->max_pds=maxp;
  array_map_with(evs, setup_cf_E, md);
  /* md->cf_E/=(double)no_etoken; */
  md->cf_E=log((double)md->cf_E);
  report(2, "Ep~cf %f (%d-%d)\n", md->cf_E, md->min_pds, md->max_pds);
}

/* ------------------------------------------------------------ */
static void write_model_file(FILE *f, model_pt m)
{
  array_pt pds=m->predicates;
  array_pt tgs=m->outcomes;
  size_t i;
  
  fprintf(f, "MET %zd %d %+12.11e\n", array_count(m->outcomes), m->max_pds, m->cf_alpha);
  for (i=0; i<array_count(pds); i++)
    {
      predicate_pt pd=(predicate_pt)array_get(pds, i);
      size_t j;

      print_predinfo(pd->data, f, tgs);
      fprintf(f, "\n");
      for (j=0; j<m->no_ocs; j++)
	{
	  feature_pt ft=(feature_pt)array_get(pd->features, j);

	  if (!ft) { continue; }
	  fprintf(f, "  %+12.11e %12d %s\n", ft->alpha, ft->count, (char *)array_get(tgs, j));
	}
    }
  fclose(f);
}

/* ------------------------------------------------------------ */
static ptrdiff_t find_tag(char *s, array_pt tgs)
{
  ptrdiff_t i;

  for (i=0; i<array_count(tgs); i++)
    { if (!strcmp(s, (char *)array_get(tgs, i))) { return i; } }
  return -1;
}

/* ------------------------------------------------------------ */
static predicate_pt read_predicate(char *s, model_pt m)
{
  array_pt tgs=m->outcomes;
  predinfo_pt pi=new_predinfo(NULL, m->no_ocs);
  size_t slen=strlen(s);
  char b[slen];
  char c[slen];

#define S_OR_B(i) (strcmp("BOUNDARY", (i)) ? register_string((i)) : NULL)
#define T_OR_B(i) (strcmp("BOUNDARY", (i)) ? array_add_unique(tgs, register_string((i))) : -1)

  while (s[slen-1]=='\n') { s[slen-1]='\0'; slen--; }
  if (1==sscanf(s, "curword=%s", b))
    { pi->type=pt_word; pi->w=register_string(b); }
  else if (1==sscanf(s, "word-1=%s", b))
    { pi->type=pt_wm1; pi->w=S_OR_B(b); }
  else if (1==sscanf(s, "word-2=%s", b))
    { pi->type=pt_wm2; pi->w=S_OR_B(b); }
  else if (1==sscanf(s, "word+1=%s", b))
    { pi->type=pt_wp1; pi->w=S_OR_B(b); }
  else if (1==sscanf(s, "word+2=%s", b))
    { pi->type=pt_wp2; pi->w=S_OR_B(b); }
  else if (1==sscanf(s, "tag-1=%s", b))
    { pi->type=pt_tm1; pi->t1=T_OR_B(b); }
  else if (2==sscanf(s, "tag-1,2=%s %s", b, c))
    { pi->type=pt_tm1tm2; pi->t1=T_OR_B(b); pi->t2=T_OR_B(c); }
  else if (1==sscanf(s, "prefix=%s", b))
    { pi->type=pt_prefix; pi->w=S_OR_B(b); }
  else if (1==sscanf(s, "suffix=%s", b))
    { pi->type=pt_suffix; pi->w=S_OR_B(b); }
  else if (!strcmp(s, "numeric"))
    { pi->type=pt_number; }
  else if (!strcmp(s, "uppercase"))
    { pi->type=pt_uppercase; }
  else if (!strcmp(s, "hyphen"))
    { pi->type=pt_hyphen; }
  else if (!strcmp(s, "DEFAULT"))
    { pi->type=pt_default; }
  else
    { error("can't read predicate \"%s\"\n", s); }

#undef S_OR_B
#undef T_OR_B
  
  return pi->predicate;
}

/* ------------------------------------------------------------ */
static void index_predicates(model_pt m)
{
  size_t i;
  array_pt pds=m->predicates;
  predindex_pt idx=new_predindex(m);
  
  /* build predicate index information (stored as m->userdata) */
  m->userdata=idx;
  for (i=0; i<array_count(pds); i++)
    {
      predicate_pt pd=array_get(pds, i);
      predinfo_pt pi=pd->data;
      /*
	report(-1, "i==%d ", i);
	print_predinfo(pi, stderr, m->outcomes);
	report(-1, "\n");
      */
      switch (pi->type)
	{
	case pt_word: hash_put(idx->word, pi->w, pd); break;
	case pt_wm1:
	  if (pi->w) { hash_put(idx->wm1, pi->w, pd); } else { idx->wm1_null=pd; }
	  break;
	case pt_wm2: 
	  if (pi->w) { hash_put(idx->wm2, pi->w, pd); } else { idx->wm2_null=pd; }
	  break;
	case pt_wp1:
	  if (pi->w) { hash_put(idx->wp1, pi->w, pd); } else { idx->wp1_null=pd; }
	  break;	  
	case pt_wp2:
	  if (pi->w) { hash_put(idx->wp2, pi->w, pd); } else { idx->wp2_null=pd; }
	  break;	  
	case pt_tm1:
	  array_set(idx->tm1, pi->t1+1, pd);
	  break;
	case pt_tm1tm2:
	  array_set((array_pt)array_get(idx->tm1tm2, pi->t1+1), pi->t2+1, pd);
	  break;
	case pt_prefix: hash_put(idx->prefix, pi->w, pd); break;
	case pt_suffix: hash_put(idx->suffix, pi->w, pd); break;
	case pt_number: idx->number=pd; break;
	case pt_uppercase: idx->uppercase=pd; break;
	case pt_hyphen: idx->hyphen=pd; break;
	case pt_default: idx->def=pd; break;
	}
    }
  report(2, "%d predicates indexed\n", array_count(pds));
}

/* ------------------------------------------------------------ */
static model_pt read_model_file(FILE *f)
{
  array_pt pds=array_new(64);
  array_pt tgs=array_new(32);
  model_pt m=new_model(tgs, pds);
  predicate_pt pd=NULL;
  char b[1024];
  size_t lno=1;
  
  if (!fgets(b, 1024, f))
    { error("can't read from model file\n"); }
  if (3!=sscanf(b, "MET %d %d %lf", &m->no_ocs, &m->max_pds, &m->cf_alpha))
    { error("can't read signature from model file\n"); }
  m->inv_max_pds=1.0/(double)m->max_pds;
  while (fgets(b, 1024, f))
    {
      lno++;
      if (b[0]=='\n') { continue; }
      if (b[0]!=' ')
	{
	  pd=read_predicate(b, m);
	  if (!pd) { error("can't read predicate in line %d\n", lno); }
	  array_add(pds, pd);
	}
      else
	{
	  feature_pt ft;
	  int c;
	  double alpha;
	  char *t=tokenizer(b, " \n");
	  
	  if (!t || 1!=sscanf(t, "%lf", &alpha))
	    { error("can't read alpha in line %d\n", lno); }
	  t=tokenizer(NULL, " \n");
	  if (!t || 1!=sscanf(t, "%d", &c))
	    { error("can't read count in line %d\n", lno); }
	  t=tokenizer(NULL, " \n");
	  if (!t || !*t)
	    { error("can't read tag in line %d\n", lno); }
	  ft=new_feature(c, array_add_unique(tgs, register_string(t)), pd);
	  m->no_fts++;
	  t=tokenizer(NULL, " \n");
	  if (t)
	    { error("additional material in line %d\n", lno); }
	  ft->alpha=alpha;
	  /* insert ft in pd */
	  array_set(pd->features, ft->outcome, ft);
	}
    }
  if (array_count(tgs)!=m->no_ocs)
    { error("no of outcomes: declared %d != found %d\n", m->no_ocs, array_count(tgs)); }
  report(1, "read %d tags, %d predicates and %d features\n",
	 m->no_ocs, array_count(pds), m->no_fts);

  index_predicates(m);

#define DEBUG_READ_MODEL_FILE 0
#if DEBUG_READ_MODEL_FILE
  {
    FILE *f=fopen("M-TT", "w");
    size_t i;
    for (i=0; i<array_count(pds); i++)
      {
	size_t j;
	pd=array_get(pds, i);
	print_predinfo(pd->data, f, tgs);
	fprintf(f, "\n");
	for (j=0; 0 && j<array_count(pd->features); j++)
	  {
	    feature_pt ft=array_get(pd->features, j);
	    if (!ft) { continue; }
	    fprintf(f, "  %+12.11e %12d %s\n", ft->alpha, ft->count, (char *)array_get(tgs, j));
	  }
      }
    fclose(f);
  }
#endif

  fclose(f);
  return m;
}

/* ------------------------------------------------------------ */
static size_t read_samples(FILE *f, array_pt sms, array_pt wds, 
			array_pt wtgs, array_pt wcs, hash_pt wdh,
			array_pt tgs)
{
  size_t lno, no_sts=0;
  char *s;

  for (lno=1, s=freadline(f); s; lno++, s=freadline(f))
    {
      char *w, *t;
      size_t wdc;
      
      for (wdc=0, w=strtok(s, " \t"); w; wdc++, w=strtok(NULL, " \t"))
	{
	  size_t ti;
	  t=strtok(NULL, " \t");
	  if (!t)
	    { report(0, "can't read tag %d in line %d\n", wdc, lno); continue; }
	  ti=array_add_unique(tgs, register_string(t));
	  w=register_word(w, ti, wds, wtgs, wcs, wdh);
	  array_add(sms, new_sample(w, ti));
	}
      if (wdc) { array_add(sms, new_sample(NULL, -1)); no_sts++; }
    }
  return no_sts;
}

#if 0
/* ------------------------------------------------------------ */
static void write_dictionary_file(FILE *f, array_pt wds, array_pt wcs,
				  array_pt wtgs, array_pt tgs)
{
  size_t i;
  
  if (!f) { return; }
  for (i=0; i<array_count(wcs); i++)
    {
      size_t wc= array_get(wcs, i);
      size_t j;
      array_pt tags;
      
      if (wc<rwt) { continue; }
      tags=(array_pt)array_get(wtgs, i);
      fprintf(f, "%s", (char *)array_get(wds, i));
/*       fprintf(f, " %zd", wc); */
      for (j=0; j<array_count(tags); j++)
	{
	  size_t ti=array_get(tags, j);
	  fprintf(f, " %s", (char *)array_get(tgs, ti));
	}
      fprintf(f, "\n");
    }
  if (f!=stdout) { fclose(f); }
}
#endif
/* ------------------------------------------------------------ */
static hash_pt read_dictionary_file(model_pt m, FILE *f, size_t cs)
{
  hash_pt d;
  char *l;
  size_t lno;
  
  if (!f) { return NULL; }
  d=hash_new(1000, .5, hash_string_hash, hash_string_equal);
  for (lno=1, l=freadline(f); l; lno++, l=freadline(f))
    {
      array_pt tgs=NULL;
      char *s, *w=tokenizer(l, " \t");
      size_t wc=0;
      
      if (!w) { continue; }
      if (!cs) { w=lowercase(w); tgs=hash_get(d, w); }
      if (!tgs)
	{ tgs=array_new(8); hash_put(d, (void *)register_string(w), (void *)tgs); }
      for (s=tokenizer(NULL, " \t"); s; s=tokenizer(NULL, " \t"))
	{
	  ptrdiff_t ti=find_tag(s, m->outcomes);

	  if (ti<0) { error("unknown tag \"%s\"\n", s); }
	  array_add(tgs, (void *)ti);
	  s=tokenizer(NULL, " \t");
	  if (!s)
	    { error("can't find tag count in line %d (old lexicon format?)\n", lno); }
	  if (1!=sscanf(s, "%td", &ti))
	    { error("can't read tag count in line %d (old format?)\n", lno); }
	  wc+=ti;
	}
    }
  fclose(f);
  report(1, "read %d lexicon entries, discarded %d entries\n", hash_size(d), lno-hash_size(d));
  return d;
}

/* ------------------------------------------------------------ */
static void count_words(void *p, void *data)
{
  ptrdiff_t c=(ptrdiff_t)p;
  int *wc=(int *)data;
  wc[0]++;
  wc[1]+=c;
  if (c<rwt) { return; }
  wc[2]++;
  wc[3]+=c;
}

/* ------------------------------------------------------------ */
static void training(FILE *mf, FILE *df, FILE *rf, size_t mi, double dt, size_t fmin)
{
  array_pt tgs=array_new(25);
  array_pt wds=array_new(1000);
  array_pt wtgs=array_new(1000);
  array_pt wcs=array_new(1000);
  hash_pt wh=hash_new(1000, .5, hash_string_hash, hash_string_equal);
  array_pt evs=array_new(5000);
  array_pt pds=array_new(1000);
  array_pt sms=array_new(5000);
  model_pt md=new_model(tgs, pds);
  size_t no_sts=read_samples(rf, sms, wds, wtgs, wcs, wh, tgs);
  double a=0.0;
  int wc[4]={0, 0, 0, 0};
  size_t i, no_sms=array_count(sms);
  
  md->no_ocs=array_count(tgs);
  array_map_with(wcs, count_words, wc);
  report(1, "%d sentences, %d samples, %d/%d words (%d/%d frequent), %d tag types\n",
	 no_sts, no_sms, wc[0], wc[1], wc[2], wc[3], md->no_ocs);
#if 0
  write_dictionary_file(df, wds, wcs, wtgs, tgs);
#endif
  array_map(wtgs, (void (*)(void *))array_free);
  array_free(wtgs); array_free(wds); wtgs=wds=NULL;
  for (i=0; i<no_sms; i++)
    {
      sample_pt s=array_get(sms, i);
      sample2event(wcs, wh, s->word, s->tag, md, evs);
      delete_sample(s);
      if (i%1000==0)
	{ report(-3, "%3d%%: %10d features\r", i*100/no_sms, md->no_fts); }
    }
  array_free(sms); array_free(wcs); hash_delete(wh);
  sms=wcs=NULL; wh=NULL;
  report(1, "%d features collected\n", md->no_fts);
  select_features(md, evs, fmin);    
  add_default_features(md, evs);
  md->inv_max_pds=1.0/(double)md->max_pds;
  report(1, "%d events (%d-%d), %d predicates\n",
	 array_count(evs), md->min_pds, md->max_pds, array_count(md->predicates));

  for (i=1; i<=mi; i++)
    {
      double na=train_iteration(md, evs);
      double delta=na-a;
      report(2, "%4d: accuracy %7.3f%%, %+9.5f%%, cf %f\n", i, na*100.0, delta*100.0, md->cf_alpha);
      a=na;
      if (i!=1 && delta<dt) { report(1, "bailing out, delta<%f\n", dt*100.0); break; }
    }

  write_model_file(mf, md);  
}

/* ------------------------------------------------------------ */
static int mycompare(const void *ip, const void *jp)
{
  int i = *((int *)ip);
  int j = *((int *)jp);

  if (dp[i] < dp[j]) { return 1; }
  if (dp[i] > dp[j]) { return -1; }
  return 0;
}

#if 0
/* ------------------------------------------------------------ */
static int predicate_matches_context(hash_pt d, predicate_pt pd, int t[], char *w[], size_t cs)
{
  predinfo_pt pi=pd->data;
  char *s=pi->w;
  /* TODO: check for case sensivity */
  void *le=hash_get(d, cs ? w[2] : lowercase(w[2]));
  
  switch (pi->type)
    {
    case pt_word: return le && !strcmp(w[2], s);
    case pt_wm1: return s==w[1] || (s && w[1] && !strcmp(w[1], s));
    case pt_wm2: return s==w[0] || (s && w[0] && !strcmp(w[0], s));
    case pt_wp1: return s==w[3] || (s && w[3] && !strcmp(w[3], s));
    case pt_wp2: return s==w[4] || (s && w[4] && !strcmp(w[4], s));
    case pt_tm1: return pi->t1==t[1];
    case pt_tm1tm2: return pi->t1==t[1] && pi->t2==t[0];
    case pt_prefix: return !le && common_prefix_length(w[2], s)==strlen(s);
    case pt_suffix: return !le && common_suffix_length(w[2], s)==strlen(s);
    case pt_number: return !le && (int)strpbrk(w[2], "0123456789");
    case pt_uppercase: return !le && (int)strpbrk(w[2], "ABCDEFGHIJKLMNOPQRSTUVWXYZÖÜÄ");
    case pt_hyphen: return !le && (int)strpbrk(w[2], "-");
    case pt_default: return 1;
    }
  return 0;
}
#endif

/* ------------------------------------------------------------ */
static void add_matching_predicates(array_pt pds, model_pt m, hash_pt d, int t[], char *w[], size_t cs)
{
  predindex_pt idx=(predindex_pt)m->userdata;
  predicate_pt pd;
  char *s=cs ? w[2] : lowercase(w[2]);

#define ARRAY_ADD_IF_NONNULL(a, p) if (p) { array_add(a, p); }

  /* TODO: check for case sensivity */
  if (hash_get(d, s))
    { pd=hash_get(idx->word, s); ARRAY_ADD_IF_NONNULL(pds, pd); }
  else
    {
      size_t cl=strlen(w[2]);
      size_t i;
      for (i=1; i<5 && i<cl; i++)
	{
	  pd=hash_get(idx->prefix, substr(w[2], 0, i)); ARRAY_ADD_IF_NONNULL(pds, pd);
	  pd=hash_get(idx->suffix, substr(w[2], cl-1, -i)); ARRAY_ADD_IF_NONNULL(pds, pd);
	}
      if (strpbrk(w[2], "0123456789"))
	{ ARRAY_ADD_IF_NONNULL(pds, idx->number); }
      if (strpbrk(w[2], "ABCDEFGHIJKLMNOPQRSTUVWXYZÖÜÄ"))
	{ ARRAY_ADD_IF_NONNULL(pds, idx->uppercase); }
      if (strpbrk(w[2], "-"))
	{ ARRAY_ADD_IF_NONNULL(pds, idx->hyphen); }
    }
  if (!w[0]) { ARRAY_ADD_IF_NONNULL(pds, idx->wm2_null); }
  else { pd=hash_get(idx->wm2, w[0]); ARRAY_ADD_IF_NONNULL(pds, pd); }

  if (!w[1]) { ARRAY_ADD_IF_NONNULL(pds, idx->wm1_null); }
  else { pd=hash_get(idx->wm1, w[1]); ARRAY_ADD_IF_NONNULL(pds, pd); }

  if (!w[3]) { ARRAY_ADD_IF_NONNULL(pds, idx->wp1_null); }
  else { pd=hash_get(idx->wp1, w[3]); ARRAY_ADD_IF_NONNULL(pds, pd); }

  if (!w[4]) { ARRAY_ADD_IF_NONNULL(pds, idx->wp2_null); }
  else { pd=hash_get(idx->wp2, w[4]); ARRAY_ADD_IF_NONNULL(pds, pd); }

  pd=array_get(idx->tm1, t[1]+1); ARRAY_ADD_IF_NONNULL(pds, pd);

  pd=array_get((array_pt)array_get(idx->tm1tm2, t[1]+1), t[0]+1);
  ARRAY_ADD_IF_NONNULL(pds, pd);
  
  ARRAY_ADD_IF_NONNULL(pds, idx->def);
#undef ARRAY_ADD_IF_NONNULL
}

/* ------------------------------------------------------------ */
/* by Tiago Tresoldi - currently, this function is never called (see line ~1087);
 * commenting it out */
/*
static void print_context(FILE *f, int t[], char *w[], array_pt tgs)
{
#define S_OR_B(i) ( (i) ? (i) : "NULL" )
#define T_OR_B(i) ( (i)<0 ? "-1" : (char *)array_get(tgs, (i)) )
  fprintf(f, "%s/%s %s/%s %s %s %s",
	  S_OR_B(w[0]), T_OR_B(t[0]), 
	  S_OR_B(w[1]), T_OR_B(t[1]), 
	  S_OR_B(w[2]), S_OR_B(w[3]), S_OR_B(w[4]));
#undef S_OR_B
#undef T_OR_B
}
*/


/* ------------------------------------------------------------ */
static void tag_probabilities(model_pt m, hash_pt d, size_t cs, int t[], char *w[], double p[], int s[])
{
  /* ok, some memory is wasted, but we avoid repetitive allocation */
  static array_pt mypds=NULL;
  array_pt tgs=m->outcomes;
  array_pt a;
  size_t no_ocs=array_count(tgs);
  size_t i;

  if (!mypds) { mypds=array_new(8); }
  
  /* hack to let the sorting function of qsort have access to p */
  dp=p;

  for (i=0; i<m->no_ocs; i++) { s[i]=i; }

/* by Tiago Tresoldi - the code below will never be executed (as for the function itself);
 * I am temporarly commenting it out to keep gcc from complaining */
/*
#define DEBUG_TAG_PROBABILITIES 0
#if DEBUG_TAG_PROBABILITIES
  print_context(stderr, t, w, m->outcomes);
#endif
*/

  array_clear(mypds);

#define USE_INDEXED_PREDICATES 1
#if USE_INDEXED_PREDICATES
  add_matching_predicates(mypds, m, d, t, w, cs);
#else
  {
    array_pt pds=m->predicates;
    for (i=0; i<array_count(pds); i++)
      {
	predicate_pt pd=array_get(pds, i);
	if (!predicate_matches_context(d, pd, t, w, cs)) { continue; }    
	array_add(mypds, pd);
      }
  }
#endif

#if DEBUG_TAG_PROBABILITIES
  for (i=0; i<array_count(mypds); i++)
    {
      predicate_pt pd=array_get(mypds, i);
      fprintf(stderr, " ");
      print_predinfo(pd->data, stderr, m->outcomes);
    }
  fprintf(stderr, "\n");
#endif
  
  assign_probabilities(m, mypds, p);
#if DEBUG_TAG_PROBABILITIES
  qsort((void *)s, no_ocs, sizeof(int), mycompare);
  report(-1, "BEFORE:");
  for (i=0; i<5; i++)
    { report(-1, " %f/%s", p[s[i]], (char *)array_get(m->outcomes, s[i])); }
  report(-1, "\nd=%p w[2]=%s dict(%s)=%p\n", d, w[2], w[2], hash_get(d, w[2]));
#endif
  if (d)
    {
      a=hash_get(d, cs ? w[2] : lowercase(w[2]));
      if (a) { redistribute_probabilities(m, a, p); }
    }
  qsort((void *)s, no_ocs, sizeof(int), mycompare);
#if DEBUG_TAG_PROBABILITIES
  report(-1, " AFTER:");
  for (i=0; i<5; i++)
    { report(-1, " %f/%s", p[s[i]], (char *)array_get(m->outcomes, s[i])); }
  report(-1, "\n");
#endif
}

#if 0
/* ------------------------------------------------------------ */
/* not used anymore, considers no sequence information */
static int tag_in_context(model_pt m, hash_pt d, int t[], char *w[], double pt, int cs)
{
  array_pt tgs=m->outcomes;
  array_pt pds=m->predicates;
  array_pt mypds=array_new(8);
  size_t no_ocs=array_count(tgs);
  double p[m->no_ocs];
  int sorter[m->no_ocs];
  size_t i;
  
  /* hack to let the sorting function of qsort have access to p */
  dp=p;
  
  for (i=0; i<no_ocs; i++) { sorter[i]=i; }
  for (i=0; i<array_count(pds); i++)
    {
      predicate_pt pd=array_get(pds, i);
      if (predicate_matches_context(d, pd, t, w, cs)) { array_add(mypds, pd); }
    }
  assign_probabilities(m, mypds, p);
  array_free(mypds);
  qsort((void *)sorter, no_ocs, sizeof(int), mycompare);

  fprintf(stdout, "%-30s", w[2]);
  for (i=0; i==0 || (pt>0.0 && p[sorter[i]]>=pt); i++)
    {
      size_t si=sorter[i];
      fprintf(stdout, " %8s %7.5f", (char *)array_get(tgs, si), p[si]);
    }
  fprintf(stdout, "\n");
  
  return sorter[0];
}
#endif

/* ------------------------------------------------------------ */
static void viterbi(model_pt m, hash_pt d, size_t cs, int t[], char *w[], size_t wno, size_t beam)
{
  size_t not=array_count(m->outcomes);
  int tgs[2]={-1, -1};
  char *wds[5]={0, 0, 0, 0, 0};
  double p[not];
  int s[not];
  double a[wno+2][not+1][not+1];
  int b[wno+2][not+1][not+1];
  double max_a;
  double b_a=-1.0;
  size_t b_i=1, b_j=1;
  size_t i;

#define DEBUG_VITERBI 0
  memset(a, 0, (wno+2)*(not+1)*(not+1)*sizeof(double));
  a[0][0][0]=1.0;
  max_a=1.0;
  for (i=0; i<wno; i++)
    {
      size_t j;
      double max_a_new=0.0;

      if (beam==0) { max_a=0.0; } else { max_a/=(double)beam; }
      for (j=0; j<2; j++) { wds[j]= i+j-2>=0 ? w[i+j-2] : NULL; }
      wds[2]=w[i];
      for (j=3; j<5; j++) { wds[j]= i+j-2<wno ? w[i+j-2] : NULL; }
      for (j=0; j<=not; j++)
	{
	  ptrdiff_t tj=j-1;
	  ptrdiff_t k;
	  for (k=0; k<=not; k++)
	    {
	      ptrdiff_t tk=k-1;
	      size_t l;
	      if (a[i][j][k]<max_a) { continue; }
	      tgs[0]=tj; 
	      tgs[1]=tk; 
	      tag_probabilities(m, d, cs, tgs, wds, p, s);
#if DEBUG_VITERBI
	      {
		double sum=0.0;
		size_t k;
		report(-1, "[%8s %8s]",
		       tgs[0]<0 ? "NULL" : (char *)array_get(m->outcomes, tgs[0]),
		       tgs[1]<0 ? "NULL" : (char *)array_get(m->outcomes, tgs[1]));
		for (k=0; k<5; k++)
		  {
		    int ti=s[k];
		    report(-1, " %8s %f", (char *)array_get(m->outcomes, ti), p[ti]);
		  }
		for (k=0; k<m->no_ocs; k++) { sum+=p[s[k]]; }
		report(-1, " --> %f\n", sum);
	      }
#endif
	      for (l=0; l<not; l++)
		{
		  double new;
		  if (p[l]==0.0) { continue; }
		  new=a[i][j][k]*p[l];
		  if (a[i+1][k][l+1]<new)
		    {
		      a[i+1][k][l+1]=new;
		      b[i+1][k][l+1]=j;
		      if (new>max_a_new) { max_a_new=new; }
		    }
		}
	    }
	}
      max_a=max_a_new;
    }
  /* find highest prob in last column */
  for (i=0; i<=not; i++)
    {
      size_t j;
      for (j=0; j<=not; j++)
	{ if (a[wno][i][j]>=b_a) { b_a=a[wno][i][j]; b_i=i; b_j=j; } }
    }
#if DEBUG_VITERBI
  report(-1, "best final state %s-%s\n",
	 b_i<=0 ? "NULL" : (char *)array_get(m->outcomes, b_i-1),
	 b_j<=0 ? "NULL" : (char *)array_get(m->outcomes, b_j-1));
#endif
  /* best final state is (b_i, b_j) */
  for (i=wno; i>0; i--)
    {
      int tmp=b[i][b_i][b_j];
      /* TODO: b_j==0 is an error (beam too small?) and should be handled differntly */
      t[i-1]= b_j==0 ? 0 : b_j-1;
      b_j=b_i;
      b_i=tmp;
    }
}

/* ------------------------------------------------------------ */
static void tag_sentence(model_pt m, hash_pt d, size_t cs, int t[], char *w[], size_t wno, size_t bw)
{
  int tgs[2]={-1, -1};
  char *wds[5]={0, 0, 0, 0, 0};
  int seq[bw][wno];
  double pseq[bw];
  double pnew[bw];
  int snew[bw];
  int tnew[bw];
  double p[m->no_ocs];
  int s[m->no_ocs];
  size_t i;

#define DEBUG_TAG_SENTENCE 0
#if DEBUG_TAG_SENTENCE
  report(-1, "wno=%d, bw=%d, m->no_ocs=%d p=%p s=%p\n", wno, bw, m->no_ocs, p, s);
  report(-1, "tgs=%p, wds=%p, seq=%p, pseq=%p, pnew=%p, snew=%p, tnew=%p\n",
	 tgs, wds, seq, pseq, pnew, snew, tnew);
#endif
  for (i=0; i<m->no_ocs; i++) { s[i]=i; }
  for (i=0; i<bw; i++) { pseq[i]=1.0; }
  for (i=0; i<wno; i++)
    {
      size_t j;
      int tmp[bw][wno];
      for (j=0; j<2; j++) { wds[j]= i+j-2>=0 ? w[i+j-2] : NULL; }
      wds[2]=w[i];
      for (j=3; j<5; j++) { wds[j]= i+j-2<wno ? w[i+j-2] : NULL; }

#if DEBUG_TAG_SENTENCE
      for (j=0; j<5; j++) { report(-1, "%s ", wds[j] ? wds[j] : "NULL"); }
      report(-1, "\n");
#endif
      
      for (j=0; j<bw; j++) { pnew[j]=0.0; snew[j]=0; tnew[j]=0; }
      for (j=0; j<bw; j++)
	{
	  size_t k;

	  /* if pseq[j]<pnew[bw-1] the jth sequence is already worse
	     (before adding this tag) than the worst in our n-best
	     list, so we can immediately ignore it */	     
	  if (pseq[j]<=pnew[bw-1]) { continue; }
	  
	  /* if pseq[j]==1.0 the seq is new and one run is sufficient */
	  if (j>0 && pseq[j]==1.0) { continue; }
	  
	  tgs[0]= i-2>=0 ? seq[j][i-2] : -1;
	  tgs[1]= i-1>=0 ? seq[j][i-1] : -1;
	  tag_probabilities(m, d, cs, tgs, wds, p, s);
#if DEBUG_TAG_SENTENCE
	  {
	    double sum=0.0;
	    report(-1, "[%8s %8s]",
		   tgs[0]<0 ? "NULL" : (char *)array_get(m->outcomes, tgs[0]),
		   tgs[1]<0 ? "NULL" : (char *)array_get(m->outcomes, tgs[1]));
	    for (k=0; k<5; k++)
	      {
		int ti=s[k];
		report(-1, " %8s %f", (char *)array_get(m->outcomes, ti), p[ti]);
	      }
	    for (k=0; k<m->no_ocs; k++) { sum+=p[s[k]]; }
	    report(-1, " --> %f\n", sum);
	  }
#endif
	  for (k=0; k<bw && k<m->no_ocs; k++)
	    {
	      int ti=s[k];
	      double pcombined=pseq[j]*p[ti];
	      ptrdiff_t l;
	      if (pcombined<=pnew[bw-1]) { continue; }
	      for (l=bw-2; l>=0 && pcombined>pnew[l]; l--)
		{ pnew[l+1]=pnew[l]; snew[l+1]=snew[l]; tnew[l+1]=tnew[l]; }
	      l++;
	      pnew[l]=pcombined; snew[l]=j; tnew[l]=ti;
#if DEBUG_TAG_SENTENCE
	      report(-1, "pnew[%d]=%e, snew[%d]=%d, tnew[%d]=%d\n", l, pnew[l], l, snew[l], l, tnew[l]);
#endif
	    }
	}
      for (j=0; j<bw; j++)
	{
	  size_t k;
	  for (k=0; k<i; k++) { tmp[j][k]=seq[snew[j]][k]; }
	  tmp[j][i]=tnew[j];
	  pseq[j]=pnew[j];
	}
      memcpy(seq, tmp, sizeof(int)*bw*wno);
#if DEBUG_TAG_SENTENCE
      for (j=0; j<bw && pseq[j]>0.0; j++)
	{
	  size_t k;
	  report(-1, "%d: %9.8e ", j, pseq[j]);
	  for (k=0; k<=i; k++)
	    { report(-1, " %8s", (char *)array_get(m->outcomes, seq[j][k])); }
	  report(-1, "\n");
	}      
#endif
    }

  for (i=0; i<wno; i++) { t[i]=seq[0][i]; }
}

/* ------------------------------------------------------------ */
static void tagging(FILE *mf, FILE *df, FILE *rf, double pt, size_t bw, size_t cs, size_t nbest)
{
  model_pt m=read_model_file(mf);
  hash_pt dic=read_dictionary_file(m, df, cs);
  char *w, *l;
  size_t wcount=32;
  char **ws=mem_malloc(sizeof(char *)*wcount);
  int *ts=mem_malloc(sizeof(int)*wcount);

  memset(ws, 0, sizeof(char *)*wcount);
  for (l=ftokenizer(rf, "\n"); l; l=ftokenizer(NULL, "\n"))
    {
      size_t i, wdc;

      for (wdc=0, w=strtok(l, " \t"); w; wdc++, w=strtok(NULL, " \t"))
	{
	  if (wdc>=wcount)
	    {
	      size_t oldcount=wcount;
	      wcount*=2;
	      ws=mem_realloc(ws, sizeof(char *)*wcount);
	      memset(&ws[oldcount], 0, sizeof(char *)*(wcount-oldcount));
	      ts=mem_realloc(ts, sizeof(int)*wcount);
	    }
	  ws[wdc]=w;
	}
      if (wdc<=0) { continue; }

      if (nbest) { tag_sentence(m, dic, cs, ts, ws, wdc, bw); }
      else { viterbi(m, dic, cs, ts, ws, wdc, bw); }

      for (i=0; i<wdc; i++)
	{
	  if (i!=0) { fprintf(stdout, " "); }
	  fprintf(stdout, "%s %s", ws[i], (char *)array_get(m->outcomes, ts[i]));
	}
      fprintf(stdout, "\n");
    }
  mem_free(ws);
  mem_free(ts);
}

/* ------------------------------------------------------------ */
static void testing(FILE *mf, FILE *df, FILE *rf, double pt, size_t bw, size_t cs, size_t nbest)
{
  model_pt m=read_model_file(mf);
  hash_pt dic=read_dictionary_file(m, df, cs);
  size_t wcount=32, pos=0, neg=0, lno, no_sts=0;
  char **ws=mem_malloc(sizeof(char *)*wcount);
  int *ts=mem_malloc(sizeof(int)*wcount);
  int *tref=mem_malloc(sizeof(int)*wcount);
  char *s;

  memset(ws, 0, sizeof(char *)*wcount);
  for (lno=1, s=ftokenizer(rf, "\n"); s; lno++, s=ftokenizer(NULL, "\n"))
    {
      char *w, *t;
      size_t wdc, i;
      
      for (wdc=0, w=strtok(s, " \t"); w; wdc++, w=strtok(NULL, " \t"))
	{
	  if (wdc>=wcount)
	    {
	      size_t oldcount=wcount;
	      wcount*=2;
	      ws=mem_realloc(ws, sizeof(char *)*wcount);
	      memset(&ws[oldcount], 0, sizeof(char *)*(wcount-oldcount));
	      ts=mem_realloc(ts, sizeof(int)*wcount);
	      tref=mem_realloc(tref, sizeof(int)*wcount);
	    }
	  t=strtok(NULL, " \t");
	  if (!t)
	    { report(0, "can't read tag %d in line %d\n", wdc, lno); break; }
	  ws[wdc]=w;
	  tref[wdc]=find_tag(t, m->outcomes);
	}
      if (wdc<=0) { continue; }
	  
      if (nbest) { tag_sentence(m, dic, cs, ts, ws, wdc, bw); }
      else { viterbi(m, dic, cs, ts, ws, wdc, bw); }

      no_sts++;
      report(-3, "%8d sentences\r", no_sts);
      for (i=0; i<wdc; i++)
	{
	  if (ts[i]==tref[i]) { pos++; }
	  else
	    {
	      report(4, "ERROR: %s ref %s guess %s\n", ws[i], 
		     (char *)array_get(m->outcomes, tref[i]),
		     (char *)array_get(m->outcomes, ts[i])); 
	      neg++;
	    }
	}
    }
  mem_free(ws);
  mem_free(ts);
  mem_free(tref);

  report(0, "%d (%d pos %d neg) words tagged, accuracy %7.3lf%%\n",
 	 pos+neg, pos, neg, 100.0*(double)pos/(double)(pos+neg));
}

/* ------------------------------------------------------------ */
static void usage(int mode)
{
  int i;
  report(-1, "\n%s\n\n%s %s\nwhere OPTIONS can be\n\n", banner, cmd, ivs[mode].call);
  for (i=0; ops[i].usage; i++)
    {
      if (strchr(ivs[mode].opt, ops[i].character))
	{ report(-1, "  %s\n", ops[i].usage); }
    }
  report(-1, "\n");
}

/* ------------------------------------------------------------ */
extern int main(int argc, char **argv)
{ 
  char c;
  FILE *mf=NULL;
  FILE *df=NULL;
  char *dictfile=NULL;
  FILE *rf=stdin;
  size_t mi=100;
  double dt=0.0;
  double pt=-1.0;
  int oldnice=nice(0), newnice=19;
  ptrdiff_t fmin=5, bw=-1, cs=0, nbest=0, mode;
  
  cmd=strdup(acopost_basename(argv[0], NULL));
  if (oldnice<0) { error("can't get priority class\n"); }

  for (mode=0; ivs[mode].cmd; mode++)
    { if (!strcmp(cmd, ivs[mode].cmd)) { break; } }
  if (!ivs[mode].cmd) { error("invalid command \"%s\"\n", cmd); }
  mode=ivs[mode].mode;

  while ((c=getopt(argc, argv, ivs[mode].opt))!=EOF)
    {
      switch (c)
	{
	case 'b':
	  if (1!=sscanf(optarg, "%td", &bw))
	    { error("invalid beam factor / n-best width \"%s\"\n", optarg); }
	  else
	    { report(1, "using %d as beam width\n", bw); }
	  break;
	case 'c':
	  cmd=strdup(optarg);
	  report(1, "running as %s\n", cmd);
	  break;
	case 'd':
	  dictfile=strdup(optarg);
	  report(1, "using %s as dictionary file\n", dictfile);
	  break;
	case 'f':
	  if (1!=sscanf(optarg, "%td", &fmin))
	    { error("invalid feature count threshold \"%s\"\n", optarg); }
	  else
	    { report(1, "using %d as feature count threshold\n", fmin); }
	  break;
	case 'h':
	  usage(mode);
	  exit(0);
	  break;
	case 'i':
	  if (1!=sscanf(optarg, "%zd", &mi))
	    { error("invalid max number of iterations \"%s\"\n", optarg); }
	  else
	    { report(1, "using %d as max number of iterations\n", mi); }
	  break;
	case 'n':
	  nbest=1;
	  report(1, "using n-best as search method\n");
	  break;
	case 'm':
	  if (1!=sscanf(optarg, "%lf", &pt))
	    { error("invalid probability threshold \"%s\"\n", optarg); }
	  else
	    { report(1, "using %f as probability threshold\n", pt); }
	  break;
	case 'p':
	  if (1!=sscanf(optarg, "%d", &newnice))
	    { error("invalid priority class \"%s\"\n", optarg); }
	  else
	    { report(1, "using %d as priority class\n", newnice); }
	  break;
	case 'r':
	  if (1!=sscanf(optarg, "%d", &rwt))
	    { error("invalid rare word threshold \"%s\"\n", optarg); }
	  else
	    { report(1, "using %d as rare word threshold\n", rwt); }
	  break;
	case 's':
	  cs=1;
	  report(1, "using case sensitive mode for dictionary\n");
	  break;
	case 't':
	  if (1!=sscanf(optarg, "%lf", &dt))
	    { error("invalid delta threshold \"%s\"\n", optarg); }
	  else
	    { report(1, "using %f as delta threshold\n", dt); dt/=100.0; }
	  break;
	case 'v':
	  if (1!=sscanf(optarg, "%d", &verbosity))
	    { error("invalid verbosity \"%s\"\n", optarg); }
	  break;
	}
    }
  if (nice(newnice-oldnice)<0) { error("can't set priority class\n"); }
  
  if (optind>=argc) { error("too few arguments\n%s", banner); }
  if (optind+1<argc) { rf=try_to_open(argv[optind+1], "r"); }
  
  if (bw<0) { bw= nbest ? 5 : 1000; }
  
  for (mode=0; ivs[mode].cmd; mode++)
    { if (!strcmp(cmd, ivs[mode].cmd)) { break; } }
  if (!ivs[mode].cmd) { error("invalid command \"%s\"\n", cmd); }
  mode=ivs[mode].mode;
  if (mode==3)
    {
      if (rwt<0) { rwt=5; }
      mf=try_to_open(argv[optind], "w");
      if (dictfile) { df=try_to_open(dictfile, "w"); }
      training(mf, df, rf, mi, dt, fmin);
    }
  else
    {
      mf=try_to_open(argv[optind], "r");
      if (dictfile) { df=try_to_open(dictfile, "r"); }
      if (mode==2)
	{ testing(mf, df, rf, pt, bw, cs, nbest); }
      else if (mode==1)
	{ tagging(mf, df, rf, pt, bw, cs, nbest); }
      else if (mode==4)
	{ error("daemon mode not implemented\n"); }
      else { error("don't know what to do (\"%s\")\n", cmd); }
    }

  exit(0);
}

/* ------------------------------------------------------------ */
