/*
  Trigram POS tagger

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

/*
  TODO:

  - try to use integers for negative logarithmic probs
    
  - use three different boundary tags instead of one
  - implement capitalization flags

  - implement multiple tags mode (easy, extend viterbi)  
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
#ifdef __APPLE__
#include <limits.h> /* MAXDOUBLE, MAXFLOAT/MacOSX */
#else
#ifdef HAVE_VALUES_H
#include <values.h> /* MAXDOUBLE, MAXFLOAT/Linux */
#endif
#endif
#include <errno.h>
#include <getopt.h>
#include "hash.h"
#include "array.h"
#include "util.h"
#include "mem.h"
#include "sregister.h"
#include "iregister.h"

/* on 64-bit systems, sizeof(void*) is different from
 * sizeof(int) so to make it compile silently we need to
 * cast first into long int and then into int. There might be
 * a better solution... */

/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */

#ifdef T3_PROB_IS_FLOAT
typedef float prob_t;
#define MAXPROB MAXFLOAT
#else
typedef double prob_t;
#define MAXPROB MAXDOUBLE
#endif

/* ------------------------------------------------------------ */

typedef struct trie_s
{
  size_t children;             /* no of children */
  unsigned char unarychar;  /* if children=1 this is the char to follow */
  struct tries_s *unarynext;/* if children=1 daughter */
  size_t count;                /* number of word tokens with this suffix */
  int *tagcount;            /* counts distinguished by tags */
  prob_t *lp;               /* smoothed lexical probabilities */
  struct trie_s *mother;    /* mother node */
  struct trie_s **next;     /* successors */
} trie_t;
typedef trie_t *trie_pt;

typedef struct word_s
{
  char *string;      /* grapheme */
  size_t count;      /* total number of occurences */
  int *tagcount;     /* maps tag index -> no. of occurences */
  prob_t *lp;
} word_t;
typedef word_t *word_pt;

typedef struct model_s
{
  iregister_pt tags;  /* lookup table tags */
  prob_t *tp;         /* smoothed transition probs */
  int *count[3];      /* uni-, bi- and trigram counts */
  int type[3];        /* uni-, bi- and trigram type counts */
  int token[3];       /* uni-, bi- and trigram token counts */
  double theta;       /* standard deviation of unconditioned ML probs */
  double lambda[3];   /* lambda_1 - _3 for trigram interpolation */
  size_t bw;    /* beam width */
  hash_pt dictionary; /* dictionary: string->array */ 
  trie_pt lower_trie; /* suffix trie for all/lowercase words */
  trie_pt upper_trie; /* suffix trie for uppercase words */
  size_t lc_count;
  size_t uc_count;
  size_t rwt;   /* rare word threshold */
  size_t msl;   /* max. suffix length */
  int stcs;  /* use one or two (case-sensitive) suffix trees */
  int stics; /* case sensitive internal in suffix trie */
  sregister_pt strings;
} model_t;
typedef model_t *model_pt;

/* ------------------------------------------------------------ */
static model_pt new_model(void)
{
  model_pt m=(model_pt)mem_malloc(sizeof(model_t));
  memset(m, 0, sizeof(model_t));
  return m;
}

/* ------------------------------------------------------------ */
static word_pt new_word(char *s, size_t cnt, size_t not)
{
  word_pt w=(word_pt)mem_malloc(sizeof(word_t));
  size_t i;

  w->string=s;
  w->count=cnt;
  w->tagcount=(int *)mem_malloc(not*sizeof(int));
  memset(w->tagcount, 0, not*sizeof(int));
  w->lp=(prob_t *)mem_malloc(not*sizeof(prob_t));
  for (i=0; i<not; i++) { w->lp[i]=-MAXPROB; }
/*   memset(w->lp, 0, not*sizeof(double)); */
  return w;
}

/* ------------------------------------------------------------ */
static void delete_word(word_pt w)
{
  mem_free(w->tagcount);
  mem_free(w->lp);
  mem_free(w);
}

/* ------------------------------------------------------------ */
trie_pt new_trie(size_t tagsnumber, trie_pt mother)
{
  trie_pt t=(trie_pt)mem_malloc(sizeof(trie_t));
  memset(t, 0, sizeof(trie_t));
  t->count=0;
  t->mother=mother;
  t->children=0;
  t->unarychar='\0';
  t->unarynext=NULL;
  t->lp=NULL;
  t->next=NULL;
  t->tagcount=(int *)mem_malloc(tagsnumber*sizeof(int));
  memset(t->tagcount, 0, tagsnumber*sizeof(int));
  return t;
}

/* ------------------------------------------------------------ */
void delete_trie(trie_pt tr)
{
  if (tr == NULL)
    return;

  if (tr->children==0) { /* nothing to do */ }
  else if (tr->children==1)
    { delete_trie((trie_pt)tr->unarynext); }
  else if (tr->next)
    {
      size_t i;
      for (i=0; i<256; i++)
	{ if (tr->next[i]) { delete_trie(tr->next[i]); } }
    }
  else
    { error("tr %p neither a leave, nor unary nor next\n", tr); }
  mem_free(tr->lp);
  mem_free(tr->tagcount);
  mem_free(tr->next);
  mem_free(tr);
}

/* ------------------------------------------------------------ */
void trie_add_daughter(trie_pt tr, unsigned char c, trie_pt daughter)
{
  if (tr->children==0)
    { tr->unarychar=c; tr->unarynext=(struct tries_s *)daughter; }
  else
    {
      if (!tr->next)
	{
	  tr->next=(trie_pt *)mem_malloc(256*sizeof(trie_pt));
	  memset(tr->next, 0, 256*sizeof(trie_pt));
	}
      if (tr->children==1) { tr->next[tr->unarychar]=(trie_pt)tr->unarynext; tr->unarynext=NULL; tr->unarychar='\0';}
      if (tr->next[c])
	{ report(1, "WARNING: tr %p c %c exists, although it shouldn't\n", tr, c); }
      tr->next[c]=daughter;
    }
  tr->children++;
}

/* ------------------------------------------------------------ */
void add_word_info_to_trie_node(model_pt m, trie_pt tr, word_pt wd)
{
  size_t i;
  size_t not = iregister_get_length(m->tags);
  tr->count += wd->count;
  for (i = 0; i < not; i++)
    tr->tagcount[i] += wd->tagcount[i];
}

/* ------------------------------------------------------------ */
trie_pt trie_get_daughter(trie_pt tr, unsigned char c)
{
  if (tr->children==1)
    { if (tr->unarychar==c) { return (trie_pt)tr->unarynext; } }
  else if (tr->next && tr->next[c]) { return tr->next[c]; }
  return NULL;
}

/* ------------------------------------------------------------ */
void add_word_to_trie(void *key, void *value, void *data)
{
  char *s=(char *)key;
  word_pt wd=(word_pt)value;
  model_pt m=(model_pt)data;
  int uc = is_uppercase(s[0]);
  trie_pt tr= uc ? m->upper_trie : m->lower_trie;
  char *t;
  size_t i;

  if (wd->count > m->rwt) { return; }
  add_word_info_to_trie_node(m, tr, wd);
  for (t = s+strlen(s)-1, i = m->msl; t >= s && i > 0; t--, i--)
    {
      unsigned char c = m->stics ? *t : tolower(*t);
      trie_pt daughter=trie_get_daughter(tr, c);

      if (!daughter)
	{
	  daughter=new_trie(iregister_get_length(m->tags), tr);
	  if (uc) { m->uc_count++; } else { m->lc_count++; }
	  trie_add_daughter(tr, c, daughter); 
	}
      tr=daughter;
      add_word_info_to_trie_node(m, tr, wd);
    }
}

/* ------------------------------------------------------------ */
/* previously inlined */
int ngram_index(size_t n, size_t s, int t1, int t2, int t3)
{
  switch (n)
    {
    case 0: return t1;
    case 1: return t1*s+t2;
    case 2: return (t1*s+t2)*s+t3;
    }
  error("ngram_index %zd>2\n", n);
  return -1; /* for the compiler */
}

/* ------------------------------------------------------------ */
void read_ngram_file(const char* fn, model_pt m)
{
  FILE *f=try_to_open(fn, "r");
  size_t lno, not;
  int t[3]={0, 0, 0};
  size_t i;
  size_t size;
  ssize_t r;
  char *s;
  char *buf = NULL;
  size_t n = 0;
  unsigned long tmp;
  
  m->tags=iregister_new(128);
  /* tag 0 is special: begin of sentence & end of sentence */
  iregister_add_unregistered_name(m->tags, "*BOUNDARY*");
  lno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      if (s[0]=='\t') { continue; }
      s=strtok(s, " \t");
      if (!s) { error("can't find tag in %s:%lu\n", fn, (unsigned long) lno); }
      iregister_add_name(m->tags, s);
    }
  not=iregister_get_length(m->tags);
  report(2, "found %d tags in \"%s\"\n", not-1, fn);

  size=sizeof(int);
  for (i=0; i<3; i++)
    {
      size*=not;
      m->count[i]=(int *)mem_malloc(size);
      memset(m->count[i], 0, size);
      m->type[i]=0;
      m->token[i]=0; 
    }

  /* rewind file */
  if (fseek(f, 0, SEEK_SET)) { error("can't rewind file \"%s\"\n", fn); }

  lno=0;
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      lno++;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      size_t cnt;
      
      for (i=0; *s=='\t'; i++, s++) { /* nada */ }
      if (i>2) { error("parse error (too many tabs) (%s:%lu)\n", fn, (unsigned long) lno); }
      s=strtok(s, " \t");
      if (!s) { error("can't find tag (%s:%lu)\n", fn, (unsigned long) lno); }
      t[i]=iregister_get_index(m->tags, s);
      if (t[i]<0) { error("unknown tag \"%s\" (%s:%lu)\n", s, fn, (unsigned long) lno); }
      s=strtok(NULL, " \t");
      if (!s) { error("can't find count (%s:%lu)\n", fn, (unsigned long) lno); }
      if (1!=sscanf(s, "%lu", &tmp)) { error("can't read count (%s:%lu)\n", fn, (unsigned long) lno); }
      cnt = tmp;
      m->count[i][ ngram_index(i, not, t[0], t[1], t[2])  ]=cnt;
      m->type[i]++;
      m->token[i]+=cnt;
    }
  report(2, "read %d/%d uni-, %d/%d bi-, and %d/%d trigram count (type/token)\n",
	 m->type[0], m->token[0], m->type[1], m->token[1], m->type[2], m->token[2]);
  if(buf!=NULL){
    free(buf);
    buf = NULL;
    n = 0;
  }
  fclose(f);
}

/* ------------------------------------------------------------ */
void compute_counts_for_boundary(model_pt m)
{
  /* compute transition probs for artificial boundary tags */
  size_t not=iregister_get_length(m->tags);
  size_t i;
  ptrdiff_t uni=0, bi=0, tri=0, ows=0, nos=0;
#define DEBUG_COMPUTE_COUNTS_FOR_BOUNDARY 0
  
  /* we don't start at zero because of the boundary tags */
  for (i=1; i<not; i++)
    {
      size_t j;
      ptrdiff_t bx, xb, bx_=0;

      bx=xb=m->count[0][ ngram_index(0, not, i, -1, -1) ];
      for (j=1; j<not; j++)
	{ 
	  size_t k;
	  ptrdiff_t bxy, xyb;

	  bx-=m->count[1][ ngram_index(1, not, j, i, -1) ];
	  xb-=m->count[1][ ngram_index(1, not, i, j, -1) ];

	  bxy=xyb=m->count[1][ ngram_index(1, not, i, j, -1) ];
	  for (k=1; k<not; k++)
	    {
	      bxy-=m->count[2][ ngram_index(2, not, k, i, j) ];
	      xyb-=m->count[2][ ngram_index(2, not, i, j, k) ];
	    }
	  bx_+=bxy;

	  m->count[2][ ngram_index(2, not, 0, i, j) ]=bxy;
	  m->count[2][ ngram_index(2, not, i, j, 0) ]=xyb;
	  tri+=bxy+xyb;
	}
      /* Boundary unigrams, two at the beginning, one at the end */
      uni+=bx+bx+xb;
      /* For each start of a sentence, there must be tags t-2, t-1. */
      m->count[1][ ngram_index(1, not, 0, 0, -1) ]+=bx;
      bi+=bx;
      /* t-1, w1 */
      m->count[1][ ngram_index(1, not, 0, i, -1) ]=bx;
      bi+=bx;
      /* wn, t+1 */
      m->count[1][ ngram_index(1, not, i, 0, -1) ]=xb;
      bi+=xb;

      /* (t-2, t-1, w1) */
      m->count[2][ ngram_index(2, not, 0, 0, i) ]=bx;
      tri+=bx;
      
      /*
	FIXME: really true?
	This compensates the lack of a continuation of the sentence.
	For bigrams, we assume that tag t+1 has a successor t+2.
      */
      m->count[1][ ngram_index(1, not, 0, 0, -1) ]+=bx;
      bi+=bx;
      /*
	FIXME: really true?
	Wrong again. There are no triples <0,0,0> of course.
	However, we added bigram counts for <0,0> for tag t+1
	(see above) and here we add corresponding (artificial)
	trigrams.
      */
      m->count[2][ ngram_index(2, not, 0, 0, 0) ]+=bx;
      tri+=bx;
      /*
	FIXME:
	Strictly speaking, the following is wrong. There's only one
	boundary tag at the end. But we need trigrams <x, 0, _>.
	See above.
	Below, the corresponding (real) bigram is added.
      */
      m->count[2][ ngram_index(2, not, i, 0, 0) ]=xb;
      tri+=xb;

      /* This is for one-word sentences: t-1, w1, t+1 */
      j=bx-bx_;
      m->count[2][ ngram_index(2, not, 0, i, 0) ]=j;
      tri+=j;
      ows+=j;
    }
  
  /* TODO: check what to use */
  m->count[0][ ngram_index(0, not, 0, -1, -1) ]=uni; /* 0? uni? uni/3? */

  nos=uni/3;
  m->token[0]+=3*nos;
  m->token[1]=m->token[0];
  m->token[2]=m->token[0];
  
#if DEBUG_COMPUTE_COUNTS_FOR_BOUNDARY 
  {
    int_t bXb=0, bXY=0, XYb=0, bbX=0, bX=0, Xb=0, i, j, k, c1, c2, c3;
    int b=m->count[0][ ngram_index(0, not, 0, -1, -1) ];
    int bb=m->count[1][ ngram_index(1, not, 0, 0, -1) ];
    int bbb=m->count[2][ ngram_index(2, not, 0, 0, 0) ];
    
    report(1, "token[0]=%d token[1]=%d token[2]=%d \n",
	   m->token[0], m->token[1], m->token[2]);
    for (i=0; i<not; i++)
      {
	bX+=m->count[1][ ngram_index(1, not, 0, i, -1) ];
	Xb+=m->count[1][ ngram_index(1, not, i, 0, -1) ];
	bbX+=m->count[2][ ngram_index(2, not, 0, 0, i) ];
	bXb+=m->count[2][ ngram_index(2, not, 0, i, 0) ];
	for (j=0; j<not; j++)
	  {
	    XYb+=m->count[2][ ngram_index(2, not, i, j, 0) ];
	    bXY+=m->count[2][ ngram_index(2, not, 0, i, j) ];
	  }
      }
    report(1, "b=%d bb=%d bbb=%d bX=%d Xb=%d bbX=%d XYb=%d bXY=%d bXb=%d\n",
	   b, bb, bbb, bX, Xb, bbX, XYb, bXY, bXb);
    for (c1=c2=c3=i=0; i<not; i++)
      {
	c1+=m->count[0][ ngram_index(0, not, i, -1, -1) ];
	for (j=0; j<not; j++)
	  {
	    c2+=m->count[1][ ngram_index(1, not, i, j, -1) ];
	    for (k=0; k<not; k++)
	      { c3+=m->count[2][ ngram_index(2, not, i, j, k) ];
	      }
	  }	
      }
    report(1, "c1=%d c2=%d c3=%d\n", c1, c2, c3);
  }
#endif
  report(1, "model generated from %d sentences (thereof %d one-word)\n", uni/3, ows);
  report(1, "found %d uni-, %d bi-, and %d trigram counts for the boundary tag\n", 
	 uni, bi, tri);
}

/* ------------------------------------------------------------ */
static void enter_rare_word_tag_counts(void *key, void *value, void *d1, void *d2)
{
  word_pt wd=(word_pt)value;
  model_pt m=(model_pt)d1;
  int *lowcount=(int *)d2;
  size_t not=iregister_get_length(m->tags);
  size_t i;

  if (wd->count>m->rwt) { return; }
  for (i=0; i<not; i++)
    { lowcount[i]+=wd->tagcount[i]; }
}

/* ------------------------------------------------------------ */
void compute_theta(model_pt m)
{
  /* TODO: check whether to include 0 */
#define START_AT_TAG 1
  size_t not=iregister_get_length(m->tags);
  int lowcount[not];
  double inv_not=1.0/(double)(not-START_AT_TAG), sum;
  size_t ttc, i;

  for (i=0; i<not; i++) { lowcount[i]=0; }
  hash_map2(m->dictionary, enter_rare_word_tag_counts, m, lowcount);
  for (ttc=0, i=START_AT_TAG; i<not; i++)
    { ttc+=lowcount[i]; }
  for (sum=0.0, i=START_AT_TAG; i<not; i++)
    {
      double x=(double)lowcount[i]/(double)ttc - inv_not;
      sum+=x*x;
    }  
  m->theta=sqrt(sum/(double)(not-START_AT_TAG-1));
  report(2, "theta %+4.3e\n", m->theta);
#undef START_AT_TAG
}

/* ------------------------------------------------------------ */
void compute_lambdas(model_pt m)
{
  size_t i, sum=0;
  size_t not=iregister_get_length(m->tags);
  int li[3]={0, 0, 0};

#define START_AT_TAG 1  
  for (i=START_AT_TAG; i<not; i++)
    {
      size_t j;
      for (j=START_AT_TAG; j<not; j++)
	{
	  int f12=m->count[1][ngram_index(1, not, i, j, 0)]-1;
	  int f2=m->count[0][ngram_index(0, not, j, 0, 0)]-1;
	  size_t k;
	  for (k=START_AT_TAG; k<not; k++)
	    {
	      ptrdiff_t f123, f23, f3, b;
	      double q[3]={0.0, 0.0, 0.0};

	      f123=m->count[2][ngram_index(2, not, i, j, k)]-1;
	      if (f123<0) { continue; }
#if 1
	      if (m->token[0]>1)
		{
		  f3=m->count[0][ngram_index(0, not, k, 0, 0)]-1;
		  q[2]=(double)f3/(double)(m->token[0]-1);
		  if (f2>0)
		    {
		      f23=m->count[1][ngram_index(1, not, j, k, 0)]-1;
		      q[1]=(double)f23/(double)f2;
		      if (f12>0) 
			{ q[0]=(double)f123/(double)f12; }
		    }
		}
	      b=0;
	      if (q[1]>q[b]) { b=1; }
	      if (q[2]>q[b]) { b=2; }
#else
 	      b=0;
	      if (f12>0) 
		{ q[0]=(double)f123/(double)f12; }
	      if (f2>0)
		{
		  f23=m->count[1][ngram_index(1, not, j, k, 0)]-1;
		  q[1]=(double)f23/(double)f2;
		  if (q[1]>q[b]) { b=1; }
		}
	      if (m->token[0]>1)
		{
		  f3=m->count[0][ngram_index(0, not, k, 0, 0)]-1;
		  q[2]=(double)f3/(double)(m->token[0]-1);
		  if (q[2]>q[b]) { b=2; }
		}
#endif
	      li[b]+=f123+1;
	      /*
	      report(2, "b==%d q0=%lf q1=%lf q2=%lf l0=%d l1=%d l2=%d\n",
		     b, q[0], q[1], q[2], li[0], li[1], li[2]);
	      */
	    }
	}
    }
  for (i=0; i<3; i++) { sum+=li[i]; }
  /* TODO: check which lambda to use. */
  for (i=0; i<3; i++) { m->lambda[i]=(double)li[2-i]/(double)sum; }  
  report(2, "lambdas: %+4.3e (%d/%d) %+4.3e (%d/%d) %+4.3e (%d/%d)\n",
	 m->lambda[0], li[2], sum,
	 m->lambda[1], li[1], sum,
	 m->lambda[2], li[0], sum);
#undef START_AT_TAG
}

/* ------------------------------------------------------------ */
void compute_transition_probs(model_pt m, int zuetp, int debugmode)
{
  size_t not=iregister_get_length(m->tags);
  /*
    FIXME
    inv_not is used for the empirical probability distributions
    if the denomiator is zero. Maybe we should simple set it
    to zero, but then \sum_0^n p(t_i | t_x, t_y) might be much
    smaller than one because one addend of the linear interpolation
    becomes zero for all tags:

    p(t_k | t_i, t_j) =
      l_1 \hat{P}(t_k) +            <--- zero if N=0 (very unlikely)
      l_2 \hat{P}(t_k | t_j) +      <--- zero if f(t_j)=0 (unlikely)
      l_3 \hat{P}(t_k | t_i, t_j)   <--- zero if f(t_i, t_j)=0 (likely)  
  */
  double inv_not= zuetp ? 0.0 : 1.0/(double)not;
  size_t i;
#define DEBUG_COMPUTE_TRANSITION_PROBS 0
  
  m->tp=(prob_t *)mem_malloc(not*not*not*sizeof(prob_t));
  memset(m->tp, 0, not*not*not*sizeof(prob_t));

  for (i=0; i<not; i++)
    {
      int ft3=m->count[0][ngram_index(0, not, i, -1, -1)];
      double pt3=m->token[0]>0 ? (double)ft3/(double)m->token[0] : inv_not;
      double l1pt3=pt3*m->lambda[0];
      size_t j;
      for (j=0; j<not; j++)
	{
	  int ft2t3=m->count[1][ngram_index(1, not, j, i, -1)];
	  int ft2=m->count[0][ngram_index(0, not, j, -1, -1)];
	  double pt3_t2=ft2>0 ? (double)ft2t3/(double)ft2 : inv_not;
	  double l2pt3_t2=pt3_t2*m->lambda[1];
	  size_t k;
	  for (k=0; k<not; k++)
	    {
	      int index=ngram_index(2, not, k, j, i);
	      int ft1t2t3=m->count[2][index];
	      int ft1t2=m->count[1][ngram_index(1, not, k, j, -1)];
	      double pt3_t1t2=ft1t2>0 ? (double)ft1t2t3/(double)ft1t2 : inv_not;
	      double l3pt3_t1t2=pt3_t1t2*m->lambda[2];
	      m->tp[index]=l1pt3 + l2pt3_t2 + l3pt3_t1t2;
#if DEBUG_COMPUTE_TRANSITION_PROBS
	      if (j==0 && k==0)
		{
		  report(-1, "%s - %s - %s\n", iregister_get_name(m->tags, k),
			 iregister_get_name(m->tags, j), iregister_get_name(m->tags, i));
		  report(-1, "  %d/%d==%4.3e %4.3e %4.3e\n",
			 ft3, m->token[0], pt3, m->lambda[0], l1pt3);
		  report(-1, "  %d/%d==%4.3e %4.3e %4.3e\n",
			 ft2t3, ft2, pt3_t2, m->lambda[1], l2pt3_t2);
		  report(-1, "  %d/%d==%4.3e %4.3e %4.3e\n",
			 ft1t2t3, ft1t2, pt3_t1t2, m->lambda[2], l3pt3_t1t2);
		  report(-1, "=> %4.3e\n", l1pt3 + l2pt3_t2 + l3pt3_t1t2);
		}
#endif	      
 	      m->tp[index]=log(m->tp[index]);
	    }
	}
    }
  report(1, "computed smoothed transition probabilities\n");

  if (debugmode)
    {
      /* bigrams and trigrams counts aren't needed anymore */
      mem_free(m->count[1]); m->count[1]=NULL;
      mem_free(m->count[2]); m->count[2]=NULL;
    }
}

/* ------------------------------------------------------------ */
static void read_dictionary_file(const char*fn, model_pt m)
{
  FILE *f=try_to_open(fn, "r");
  char *s, *rs;
  size_t lno, no_token=0;
  size_t not=iregister_get_length(m->tags);
  ssize_t r;
  char *buf = NULL;
  size_t n = 0;
  unsigned long tmp;
  
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
      
      s=strtok(s, " \t");
      if (!s) { report(1, "can't find word (%s:%lu)\n", fn, (unsigned long) lno); continue; }
      rs=(char*)sregister_get(m->strings,s);
      wd=new_word(rs, 0, not);
      old=hash_put(m->dictionary, rs, wd);
      if (old)
	{
	  report(1, "duplicate dictionary entry \"%s\" (%s:%lu)\n", s, fn, (unsigned long) lno);
	  delete_word(old);
	}
      for (s=strtok(NULL, " \t"); s;  s=strtok(NULL, " \t"))
	{
	  ptrdiff_t fti;
	  ptrdiff_t ti=iregister_get_index(m->tags, s);
	  
	  if (ti<0)
	    { report(0, "invalid tag \"%s\" (%s:%lu)\n", s, fn, (unsigned long) lno); continue; }
	  s=strtok(NULL, " \t");
	  if (!s || 1!=sscanf(s, "%lu", &tmp))
	    { report(1, "can't find tag count (%s:%lu)\n", fn, (unsigned long) lno); continue; }
	  cnt = tmp;
	  wd->count+=cnt;
	  wd->tagcount[ti]=cnt;
	  fti=m->count[0][ ngram_index(0, not, ti, -1, -1) ];
	  if (fti<=0) { error("invalid frequency count for \"%s\"\n", s); }
	  wd->lp[ti]=(double)cnt/(double)fti;
	  wd->lp[ti]=log(wd->lp[ti]);
	}
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
void count_nodes(trie_pt tr, int s[])
{
  /* 0 leaves, 1 unary branching, 2 total */
  s[2]++;
  if (tr->children==0) { s[0]++; }
  else if (tr->children==1)
    { s[1]++; count_nodes((trie_pt)tr->unarynext, s); }
  else if (tr->next)
    {
      size_t i;
      for (i=0; i<256; i++)
	{ if (tr->next[i]) { count_nodes(tr->next[i], s); } }
    }
  else
    { error("tr %p neither a leave, nor unary nor next\n", tr); }
}

/* ------------------------------------------------------------ */
void build_suffix_trie(model_pt m)
{
  m->lower_trie=new_trie(iregister_get_length(m->tags), NULL);
  m->upper_trie=new_trie(iregister_get_length(m->tags), NULL);

  hash_map1(m->dictionary, add_word_to_trie, m);
  report(1, "built suffix tries with %d lowercase and %d uppercase nodes\n",
	 m->lc_count, m->uc_count);

#if 1
  {
    int c[]={0, 0, 0};
    count_nodes(m->lower_trie, c);
    report(1, "leaves/single/total LC: %d %d %d\n", c[0], c[1], c[2]);
    c[0]=0; c[1]=0; c[2]=0;
    count_nodes(m->upper_trie, c);
    report(1, "leaves/single/total UC: %d %d %d\n", c[0], c[1], c[2]);
  }    
#endif
}

/* ------------------------------------------------------------ */
static void smooth_suffix_probs(model_pt m, trie_pt tr, trie_pt dad, int debugmode)
{
  size_t not=iregister_get_length(m->tags);
  double one_plus_theta=1.0+m->theta;
  size_t i;

  tr->lp=(prob_t *)mem_malloc(not*sizeof(prob_t));
  memset(tr->lp, 0, not*sizeof(prob_t));
  for (i=0; i<not; i++)
    {
      int tc=tr->tagcount[i];
      double p=0.0;
      if (tc>0)
	{
	  p=(double)tc/(double)tr->count;
	  /*
	    p is an estimate of P(t_i | w). However, for Viterbi
	    we need P(w | t_i).

	    Using Bayes' formula:
	      P(w | t_i) = P(w) * P(t_i | w)/P(t_i)
	    P(w) is constant:
	      P(w | t_i) = P(t_i | w)/P(t_i)		    
	    P(t_i) = f(t_i)/N and N is constant:
	      P(w | t_i) = P(t_i | w)/f(t_i)
	    That means that we have to divide by f(t_i).		    

	    f(t_i) cannot be zero because we already saw words with that
	    tag, e. g. this one.
	  */
	  p/=m->count[0][ ngram_index(0, not, i, -1, -1) ]; 
	}
      if (dad) { p+=m->theta*dad->lp[i]; p/=one_plus_theta; }
      tr->lp[i]=p;
    }

  if (!debugmode) { mem_free(tr->tagcount); tr->tagcount=NULL; }

  if (tr->children==1) { smooth_suffix_probs(m, (trie_pt)tr->unarynext, tr, debugmode); }
  else if (tr->next)
    {
      for (i=0; i<256; i++)
	{
	  trie_pt son=tr->next[i];
	  if (!son) { continue; }
	  smooth_suffix_probs(m, son, tr, debugmode);
	}
    }

  for (i=0; i<not; i++) { tr->lp[i]=log(tr->lp[i]); }
}

/* ------------------------------------------------------------ */
void compute_unknown_word_probs(model_pt m, int debugmode)
{
  if (m->lower_trie) { smooth_suffix_probs(m, m->lower_trie, NULL, debugmode); }
  if (m->upper_trie) { smooth_suffix_probs(m, m->upper_trie, NULL, debugmode); }
  report(1, "suffix probabilities smoothing done [theta %4.3e]\n", m->theta);
}

/* ------------------------------------------------------------ */
trie_pt lookup_suffix_in_trie(trie_pt tr, char *s)
{
  char *t=s+strlen(s)-1;
  trie_pt d;
  
  for (; t>=s && (d=trie_get_daughter(tr, *t)); t--) { tr=d; }
  return tr;
}

/* ------------------------------------------------------------ */
prob_t *get_lexical_probs(model_pt m, char *s)
{
  word_pt w=hash_get(m->dictionary, s);

  if (w) { return w->lp; }
  else
    {
      trie_pt tr= is_uppercase(s[0]) ? m->upper_trie : m->lower_trie;

      tr=lookup_suffix_in_trie(tr, s);
      return tr->lp;
    }
}

/* ------------------------------------------------------------ */
/*
  Extend viterbi() so that it can also work in multiple-tags
  mode.
  - Add parameter ``prob_t probs[]''.
  - if probs==NULL we are in best-sequence mode.
  - if probs!=NULL we are in multi-tag mode
    Probs is then a pre-allocated C-array of size not*wno.
  - The probability that word w_i has tag t_j is stored in
    probs[i*not+j].
  - Change declaration of a to ``prob_t a[wno][not][not]''
  - Adapt updating of nai and cai.
  - When finished, the traverse b[][][] and a[][][] and enter
    infos in probs.
*/
void viterbi(model_pt m, array_pt words, array_pt tags)
{
  size_t i, j, k, l, cai, nai=0;
  size_t not=iregister_get_length(m->tags);
  size_t wno=array_count(words);
  prob_t a[2][not][not];
  array_pt b = array_new(wno);
  prob_t max_a;
  prob_t b_a=-MAXPROB;
  ptrdiff_t b_i=1, b_j=1;
  array_pt arr_b_i;
  int *arr_b_k;
  int *intarrs = calloc(wno*not*not, sizeof(int));

  for (i = 0; i < wno; ++i) {
	  array_pt b2 = array_new(not);
	  array_set(b, i, (void*) b2);
	  for (k = 0; k < not; ++k) {
		  int *arr = intarrs + i*not + k*wno*not;
		  array_set(b2, k, (void*) arr);
	  }
  }

#define DEBUG_VITERBI 0
  for (j=0; j<not; j++)
    { for (k=0; k<not; k++) { a[0][j][k]=-MAXPROB; } }

  a[0][0][0]=0.0;
  max_a=0.0;
  for (i=0; i<wno; i++)
    {
      prob_t max_a_new=-MAXPROB;
      char *w=(char *)array_get(words, i);
      prob_t *lp=get_lexical_probs(m, w);

      cai=i%2; nai= cai==0 ? 1 : 0;

      /* clear next a column */
      for (j=0; j<not; j++)
	{ for (k=0; k<not; k++) { a[nai][j][k]=-MAXPROB; } }

      /* TODO: precompute log(m->bw) */
      if (m->bw==0) { max_a=-MAXPROB; } else { max_a-=log((prob_t)m->bw); }
      for (l=0; l<not; l++)
	{
	  if (lp[l]==-MAXPROB) { continue; }
	  for (j=0; j<not; j++)
	    {
	      for (k=0; k<not; k++)
		{
		  prob_t new;
		  if (a[cai][j][k]<max_a) { continue; }
		  new=a[cai][j][k] + m->tp[ ngram_index(2, not, j, k, l) ] + lp[l];
#if DEBUG_VITERBI
#define TN(x) iregister_get_name(m->tags, x)
		  report(-1, "Considering <%s-%s> --> <%s-%s> for %s\n",
			 TN(j), TN(k), TN(k), TN(l), w);
		  report(-1, "\ta(%s-%s)==%5.4e\n", TN(j), TN(k), a[cai][j][k]);
		  report(-1, "\tlp(%s|%s)==%5.4e\n", TN(l), w, lp[l]);
		  report(-1, "\ttp(%s-%s --> %s-%s)==%5.4e\n",
			 TN(j), TN(k), TN(k), TN(l),
			 m->tp[ ngram_index(2, not, j, k, l) ]);
		  report(-1, "\t---> %5.4e\n", new);
#endif
		  if (new>a[nai][k][l])
		    {
		      a[nai][k][l]=new;
		      arr_b_i = (array_pt) array_get(b, i);
		      arr_b_k = (int*) array_get(arr_b_i, k);
		      arr_b_k[l]=j;
		      if (new>max_a_new) { max_a_new=new; }
		    }
		}
	    }
	}
      max_a=max_a_new;
    }

  /* find highest prob in last column */
  for (i=0; i<not; i++)
    {
      size_t j;
      for (j=0; j<not; j++)
	{
	  /*
	    FIXME:
	    Should we use bigrams here? Cf. Brants (2000) page 1.
	    prob_t new=a[nai][i][j] + m->tp[ ngram_index(1, not, j, 0, -1) ];	  
	  */
	  prob_t new=a[nai][i][j] + m->tp[ ngram_index(2, not, i, j, 0) ];
#if DEBUG_VITERBI
	  if (a[nai][i][j]>-MAXPROB)
	    {
	      report(-1, "Considering <%s-%s> as best final state\n", TN(i), TN(j));
	      report(-1, "\ta(%s-%s)==%5.4e\n", TN(i), TN(j), a[nai][i][j]);
	      report(-1, "\ttp(%s-%s --> %s-%s)==%5.4e\n",
		     TN(i), TN(j), TN(j), "NULL", m->tp[ ngram_index(2, not, i, j, 0) ]);
	      report(-1, "\t---> %5.4e\n",
		     a[nai][i][j] * m->tp[ ngram_index(2, not, i, j, 0) ]);
	    }
#endif
	  if (new>b_a) { b_a=new; b_i=i; b_j=j; }
	}
    }

#if DEBUG_VITERBI
  report(-1, "best final state %s-%s\n",
	 (char *)iregister_get_name(m->tags, b_i),
	 (char *)iregister_get_name(m->tags, b_j));
#endif

  /* best final state is (b_i, b_j) */
  for (i=wno; i>0; )
    {
            i--;
	    arr_b_i = (array_pt) array_get(b, i);
	    arr_b_k = (int*) array_get(arr_b_i, b_i);
	    size_t tmp=arr_b_k[b_j];
      array_set(tags, i, (void *)b_j);
      b_j=b_i;
      b_i=tmp;
    }

  for (i = 0; i < wno; ++i) {
	  array_pt b2 = array_get(b, i);
	  array_free(b2);
  }

  free(intarrs);
  array_free(b);
}


#if 0
/* ------------------------------------------------------------ */
void forward_backward(model_pt m, array_pt words, array_pt tags)
{
  size_t not=iregister_get_length(m->tags);
  size_t wno=array_count(words);
  /* forward probs alpha */
  prob_t a[wno+2][not][not];
  /* backward probs beta */
  prob_t b[wno+2][not][not];
  prob_t max_a;
  prob_t b_a=-MAXPROB;
  ptrdiff_t i, j, k, l;

#define DEBUG_ML 0
  /* forward variables */
  for (j=0; j<not; j++)
    { for (k=0; k<not; k++) { a[0][j][k]=-MAXPROB; } }
  a[0][0][0]=0.0;
  for (i=0; i<wno; i++)
    {
      size_t ip=i+1;
      char *w=(char *)array_get(words, i);
      prob_t *lp=get_lexical_probs(m, w);

      /* clear next a column */
      for (k=0; k<not; k++)
	{ for (l=0; l<not; l++) { a[ip][k][l]=-MAXPROB; } }

      for (l=0; l<not; l++)
	{
	  if (lp[l]==-MAXPROB) { continue; }
	  for (k=0; k<not; k++)
	    {
	      prob_t new=-MAXPROB;
	      for (j=0; j<not; j++)
		{
		  prob_t a_ijk=a[i][j][k];
		  prob_t tp=m->tp[ ngram_index(2, not, j, k, l) ];
		  new=log_prob_add(new, a_ijk+tp);
		}
	      a[ip][k][l]=new + lp[l];
	    }
	}
    }
  /* backward variables */
  for (j=0; j<not; j++)
    { for (k=0; k<not; k++) { b[wno+1][j][k]=-MAXPROB; } }
  b[wno+1][0][0]=0.0;
  for (i=wno-1; i>=0; i--)
    {
      size_t ip=i+1;
      char *w=(char *)array_get(words, i);
      prob_t *lp=get_lexical_probs(m, w);

      /* clear next a column */
      for (k=0; k<not; k++)
	{ for (l=0; l<not; l++) { b[ip][k][l]=-MAXPROB; } }

      for (l=0; l<not; l++)
	{
	  if (lp[l]==-MAXPROB) { continue; }
	  for (k=0; k<not; k++)
	    {
	      prob_t new=-MAXPROB;
	      for (j=0; j<not; j++)
		{
		  prob_t a_ijk=a[i][j][k];
		  prob_t tp=m->tp[ ngram_index(2, not, j, k, l) ];
		  new=log_prob_add(new, a_ijk+tp);
		}
	      a[ip][k][l]=new + lp[l];
	    }
	}
    }
}
#endif

/* ------------------------------------------------------------ */
void debugging(model_pt m)
{
  size_t not=iregister_get_length(m->tags);
  int ts[3]={-1, -1, -1};
  char *s;
  ssize_t r;
  char *buf = NULL;
  size_t n = 0;

  report(-1, "Entering debug mode...\n");
  while ((r = readline(&buf,&n,stdin)) != -1)
    {
      s = buf;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      char *t;
      size_t i, j, mode;
      for (i=0, t=strtok(s, " \t"), mode=0; t && i<3 && mode==0; i++, t=strtok(NULL, " \t"))
	{
	  ts[i]=iregister_get_index(m->tags, t);
	  if (!strcmp(t, "NULL")) { ts[i]=0; }
	  if (ts[i]<0) { mode=1; }
	}
      if (mode==0)
	{
	  i--; 
	  report(-1, "TP ");
	  for (j=0; j<=i; j++)
	    {
	      report(-1, "%s(%4.3e) ",
		     iregister_get_name(m->tags, ts[j]),
		     (double)m->count[0][ ngram_index(0, not, ts[j], -1, -1) ]/m->token[0]);
	    }
	  if (i>0)
	    {
	      report(-1, "[%s-%s %8.7e] ",
		     iregister_get_name(m->tags, ts[i-1]), iregister_get_name(m->tags, ts[i]),
		     (double)m->count[1][ ngram_index(1, not, ts[i-1], ts[i], -1) ]/
		     m->count[0][ ngram_index(0, not, ts[i-1], -1, -1) ]);
	    }
	  if (i>1)
	    {
	      report(-1, "[%s-%s-%s %8.7e] ", 
		     iregister_get_name(m->tags, ts[0]), iregister_get_name(m->tags, ts[1]), iregister_get_name(m->tags, ts[2]),
		     (double)m->count[2][ ngram_index(2, not, ts[0], ts[1], ts[2]) ]/
		     m->count[1][ ngram_index(1, not, ts[0], ts[1], -1) ]);
	      report(-1, "[smoothed %12.11e] ", m->tp[ ngram_index(i, not, ts[0], ts[1], ts[2]) ]);
	    }
	  report(-1, "\n");	  
	}
      else
	{
	  while (strtok(NULL, " \t")) { /* nada */ }
	  for (t=strtok(s, " \t"); t; t=strtok(NULL, " \t"))
	    {
	      prob_t *p=get_lexical_probs(m, t);
	      word_pt w=hash_get(m->dictionary, t);
	      size_t i, j;

	      if (w)
		{	      
		  report(-1, "LEXICON %s ", t);
		  for (i=j=0; i<iregister_get_length(m->tags); i++)
		    {
		      if (p[i]==-MAXPROB) { continue; }
		      report(-1, "  [%s %3.2e]", (char *)iregister_get_name(m->tags, i), p[i]);
		    }
		  report(-1, "\n");
		}
	      else
		{
		  trie_pt tr= is_uppercase(t[0]) ? m->upper_trie : m->lower_trie;
	      
		  tr=lookup_suffix_in_trie(tr, t);
/* 		  report(-1, "SUFFIX %s \"%s\"\n", t, trie_string(tr)); */
		  report(-1, "SUFFIX %s \"%s\"\n", t, "*UNKNOWN*");
		  for (i=j=0; i<iregister_get_length(m->tags); i++)
		    {
		      if (p[i]==-MAXPROB || tr->tagcount[i]==0) { continue; }
		      j++;
		      report(-1, "  [%s %3.2e %d]",
			     (char *)iregister_get_name(m->tags, i), p[i], tr->tagcount[i]);
		      if (j%4==0) { report(-1, "\n"); }
		    }
		  report(-1, "\n");
		}
	    }
	}
    }
  if(buf!=NULL){
    free(buf);
    buf = NULL;
    n = 0;
  }
}

/* ------------------------------------------------------------ */
void dump_transition_probs(model_pt m)
{
  size_t not=iregister_get_length(m->tags);
  size_t i, j, k;

  for (i=0; i<not; i++)
    {
      for (j=0; j<not; j++)
	{
	  for (k=0; k<not; k++)
	    {
	      int index=ngram_index(2, not, i, j, k);
	      fprintf(stdout, "tp(%s,%s => %s)=%12.11e\n",
		      (char *)iregister_get_name(m->tags, i),
		      (char *)iregister_get_name(m->tags, j),
		      (char *)iregister_get_name(m->tags, k),
		      exp(m->tp[index]));
	      fprintf(stdout, "tri(%s,%s,%s)=%d\n",
		      (char *)iregister_get_name(m->tags, i),
		      (char *)iregister_get_name(m->tags, j),
		      (char *)iregister_get_name(m->tags, k),
		      m->count[2][ ngram_index(2, not, i, j, k) ]);
	    }
	  fprintf(stdout, "bi(%s,%s)=%d\n",
		  (char *)iregister_get_name(m->tags, i),
		  (char *)iregister_get_name(m->tags, j),
		  m->count[1][ ngram_index(1, not, i, j, -1) ]);
	}
      fprintf(stdout, "uni(%s)=%d\n",
		      (char *)iregister_get_name(m->tags, i),
		      m->count[0][ ngram_index(0, not, i, -1, -1) ]);
    }
}

/* ------------------------------------------------------------ */
void tag_sentence(model_pt m, array_pt words, array_pt tags, char *l)
{
  char *t;
  size_t i;
  array_clear(words); array_clear(tags);
  for (t=strtok(l, " \t"); t; t=strtok(NULL, " \t"))
    { array_add(words, t); }
  viterbi(m, words, tags);
  for (i=0; i<array_count(words); i++)
    {
      size_t ti=(size_t)array_get(tags, i);
      char *tn=(char *)iregister_get_name(m->tags, ti);
      char *wd=(char *)array_get(words, i);
      if (i>0) { fprintf(stdout, " "); }
      fprintf(stdout, "%s %s", wd, tn);
    }
  fprintf(stdout, "\n");
}

/* ------------------------------------------------------------ */
static void tagging(const char* fn, int bmode, model_pt m)
{
  FILE *f= fn ? try_to_open(fn, "r") : stdin;
  array_pt words=array_new(128), tags=array_new(128);
  char *s;
  ssize_t r;
  char *buf = NULL;
  size_t n = 0;

  if (bmode>=0 && !setvbuf(f, NULL, bmode, 0))
    { report(0, "setvbuf error: %s\n", strerror(errno)); }
  while ((r = readline(&buf,&n,f)) != -1)
    {
      s = buf;
      if (r>0 && s[r-1]=='\n') s[r-1] = '\0';
      if(r == 0) { continue; }
      tag_sentence(m, words, tags, s);
    }
  array_free(words); array_free(tags);
  if(buf!=NULL){
    free(buf);
    buf = NULL;
    n = 0;
  }
  if (fn) {
    fclose(f);
  }
}

/* ------------------------------------------------------------ */
void testing(const char* fn, model_pt m)
{
  FILE *f= fn ? try_to_open(fn, "r") : stdin;  
  array_pt words=array_new(128), tags=array_new(128), refs=array_new(128);
  char *l;
  ssize_t r;
  size_t pos=0, neg=0;
  char *buf = NULL;
  size_t n = 0;
  
  while ((r = readline(&buf,&n,f)) != -1)
    {
      l = buf;
      if (r>0 && l[r-1]=='\n') l[r-1] = '\0';
      char *t;
      size_t i;

      array_clear(words); array_clear(tags); array_clear(refs);
      for (t=strtok(l, " \t"), i=0; t; t=strtok(NULL, " \t"), i++)
	{
	  if (i%2==0) { array_add(words, t); }
	  else
	    {
	      ptrdiff_t ti=iregister_get_index(m->tags, t);
	      if (ti<0) { error("unknown tag \"%s\"\n", t); }
              array_add(refs, (void *)ti);
	    }
	}
      if (array_count(words)==0) { continue; }
      viterbi(m, words, tags);
      for (i=0; i<array_count(words); i++)
	{
	  size_t guess=(size_t)array_get(tags, i);
	  size_t ref=(size_t)array_get(refs, i);
	  if (guess==ref) { pos++; }
	  else
	    {
	      if (verbosity>3)
		{
		  const char *gs=iregister_get_name(m->tags, guess);
		  const char *rs=iregister_get_name(m->tags, ref);
		  size_t j;
		  for (j=i; j!=i+2; j++)
		    {
		      if (j<2) { continue; }
		      report(-1, "%s %s ", (char *)array_get(words, j-2),
			     (char *)iregister_get_name(m->tags, (size_t)array_get(refs, j-2)));
		    }
		  report(-1, ":: %s guess %s ref %s\n", (char *)array_get(words, i), gs, rs);
		}
	      neg++;	      
	    }
	}
    }
  array_free(words); array_free(tags); array_free(refs);
  report(0, "%d (%d+%d) words tagged, accuracy %7.3f%%\n",
	 pos+neg, pos, neg, 100.0*(double)pos/(double)(pos+neg));
  if(buf!=NULL){
    free(buf);
    buf = NULL;
    n = 0;
  }
  if (fn) {
    fclose(f);
  }
}

/* ------------------------------------------------------------ */
void delete_model(model_pt m)
{
  hash_iterator_pt hi;
  void *key;
  size_t i;

  /* Delete the count array. */
  for (i = 0; i < 3; ++i) {
    mem_free(m->count[i]);
  }

  /* Delete the tags lookup register. */
  iregister_delete(m->tags);

  /* Delete the probabilities. */
  mem_free(m->tp);

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

  /* Delete the tries. */
  delete_trie(m->lower_trie);
  delete_trie(m->upper_trie);

  /* Free strings register */
  sregister_delete(m->strings);

  /* Delete the model itself. */
  mem_free(m);
}

static int lambdas_parser(char* arg, void* lambdas) {
	double* l = (double*) lambdas;
	if (3!=sscanf(arg, "%lf %lf %lf", l, l+1, l+2))
	{
		return 1;
	}
	return 0;
}

static int lambdas_serializer(void* lambdas, FILE* out) {
	double* l = (double*) lambdas;
	fprintf(out, "%lf %lf %lf", l[0], l[1], l[2]);
	return 0;
}

/* ------------------------------------------------------------ */
int main(int argc, char **argv)
{
  model_pt model=new_model();

  int h = 0;
  unsigned long v = 1;
  long r = 1;
  double s = -1.0;
  long L = 10;
  long b = 0;
  int Z = 0;
  int x = 0;
  int y = 0;
  int z = 0;
  char *l = NULL;
  double a[3];
  a[0] = -1.0;
  a[1] = -1.0;
  a[2] = -1.0;
  option_callback_data_t cdlambdas = {
    a,
    lambdas_parser,
    lambdas_serializer
  };
  enum OPTION_OPERATION_MODE o = OPTION_OPERATION_TAG;
  option_callback_data_t cd = {
    &o,
    option_operation_mode_parser,
    option_operation_mode_serializer
  };
  option_context_t options = {
	  argv[0],
	  "trigram-based part-of-speech tagger",
	  "OPTIONS modelfile [inputfile]",
	  version_copyright_banner,
	  (option_entry_t[]) {
		  { 'h', OPTION_NONE, (void*)&h, "display this help" },
		  { 'v', OPTION_UNSIGNED_LONG, (void*)&v, "verbosity level [1]" },
		  { 'r', OPTION_SIGNED_LONG, (void*)&r, "rare word threshold [0]" },
		  { 'l', OPTION_STRING, (void*)&l, "lexicon file [none]" },
		  { 'Z', OPTION_NONE, (void*)&Z, "use line-buffered IO for input" },
		  { 'x', OPTION_NONE, (void*)&x, "case-insensitive suffix tries [sensitive]" },
		  { 'y', OPTION_NONE, (void*)&y, "case-insensitive when branching in suffix trie [sensitive]" },
		  { 'z', OPTION_NONE, (void*)&z, "zero empirical transition probs if undefined [1/#tags]" },
		  { 'o', OPTION_CALLBACK, (void*)&cd, "mode of operation 0/tag, 1/test, 7/dump, 8/debug [tag]" },

		  { 'a', OPTION_CALLBACK, (void*)&cdlambdas, "transition smoothing lambdas" },
		  { 'b', OPTION_SIGNED_LONG, (void*)&b, "beam factor [1000]" },
		  { 'L', OPTION_SIGNED_LONG, (void*)&L, "maximum suffix length [10]" },
		  { 's', OPTION_DOUBLE, (void*)&s, "theta for suffix backoff [SD of tag probabilities]" },
		  { '\0', OPTION_NONE, NULL, NULL }
	  }
  };
  int idx = options_parse(&options, "--", argc, argv);
  if(h) {
	  options_print_usage(&options, stdout);
	  return 0;
  }
  if(l == NULL) {
	  options_print_usage(&options, stderr);
	  error("missing lexicon file\n");
  }
  char *mf = NULL;
  char *ipf = NULL;
  if (idx<argc)
  {
	  mf=argv[idx];
	  idx++;
  } else {
	  options_print_usage(&options, stderr);
	  error("missing rules file\n");
  }
  if (idx<argc && strcmp("-", argv[idx]))
  {
	  ipf=argv[idx];
  }
  if(o!=OPTION_OPERATION_TAG && o!=OPTION_OPERATION_TEST && o!=OPTION_OPERATION_DUMP && o!=OPTION_OPERATION_DEBUG)
  {
	  error("invalid mode of operation \"%d\"\n", o);
  }
  if(v >= 1) {
	  options_print_configuration(&options, stderr);
  }
  model->rwt = r;
  model->stcs = !x;
  model->stics = !y;

  model->strings = sregister_new(500);


  read_ngram_file(mf, model);
  compute_counts_for_boundary(model);

  model->bw = b;
  if (a[0]<0.0) { compute_lambdas(model); }
  else { int i; for (i=0; i<3; i++) { model->lambda[i]=a[i]; } }
  compute_transition_probs(model, z, (o == OPTION_OPERATION_DEBUG));


  read_dictionary_file(l, model);
  if (s<0.0) { compute_theta(model); }
  else { model->theta=s; }
  build_suffix_trie(model);
  compute_unknown_word_probs(model, (o == OPTION_OPERATION_DEBUG));

  switch (o)
    {
    case OPTION_OPERATION_TAG:
      /* _IOFBF fully buffered; _IOLBF line buffered; _IONBF not buffered */
      tagging(ipf, Z ? _IOLBF : -1, model); break;
    case OPTION_OPERATION_TEST:
      testing(ipf, model); break;
    case OPTION_OPERATION_DUMP:
      dump_transition_probs(model); break; 
    case OPTION_OPERATION_DEBUG:
      debugging(model); break;
/*   case 9: sleep(30); break; */
    default:
      report(0, "unknown mode of operation\n");
    }

  report(1, "done\n");

  delete_model(model);

  exit(0);
}

/* ------------------------------------------------------------ */
