/*
  Generalized Iterative Scaling

  Copyright (c) 2001-2002, Ingo SchrÃ¶der
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "array.h"
#include "mem.h"
#include "gis.h"
#include "util.h"

/* ------------------------------------------------------------ */
event_pt new_event(int count, int oc)
{
  event_pt e=(event_pt)mem_malloc(sizeof(event_t));
  e->count=count;
  e->outcome=oc;
  e->predicates=array_new(8);
  return e;
}

/* ------------------------------------------------------------ */
predicate_pt new_predicate(int count, void *data, int no_ocs)
{
  predicate_pt p=(predicate_pt)mem_malloc(sizeof(predicate_t));
  p->count=count;
  p->data=data;
  p->features=array_new_fill(no_ocs, NULL);
  return p;
}

/* ------------------------------------------------------------ */
void delete_predicate(predicate_pt pd)
{
  if (pd->features) { array_free(pd->features); }
  mem_free(pd);
}

/* ------------------------------------------------------------ */
feature_pt new_feature(int count, int oc, predicate_pt pd)
{
  feature_pt ft=(feature_pt)mem_malloc(sizeof(feature_t));

  ft->count=count;
  ft->outcome=oc;
  ft->predicate=pd;
  ft->E=ft->alpha=ft->mod=0.0;
  return ft;
}

/* ------------------------------------------------------------ */
void delete_feature(feature_pt ft)
{
  mem_free(ft);
}

/* ------------------------------------------------------------ */
model_pt new_model(array_pt ocs, array_pt pds)
{
  model_pt md=(model_pt)mem_malloc(sizeof(model_t));
  md->outcomes=ocs;
  md->predicates=pds;
  md->no_fts=md->no_ocs=0;
  md->min_pds=md->max_pds=0;
  md->inv_max_pds=md->cf_alpha=md->cf_E=0.0;
  md->userdata=NULL;
  return md;
}

/* ------------------------------------------------------------ */
double train_iteration(model_pt m, array_pt evs)
{
  int no_ocs=m->outcomes ? array_count(m->outcomes) : 0;
  int no_evs=array_count(evs);
  int modulo=no_evs/20;
  double pab[no_ocs];
  int corr[no_ocs];  
  double cf_mod=0.0;  
  int pos=0, neg=0, i;

  for (i=0; i<no_evs; i++)
    {      
      event_pt ev=(event_pt)array_get(evs, i);
      double psum=0.0, b_pab;
      int j, b_oc;
      
      if (i%modulo==0) { report(3, "%3d%%\r", i*100/no_evs); }
      for (j=0; j<no_ocs; j++) { pab[j]=0.0; corr[j]=m->max_pds; }
      for (j=0; j<array_count(ev->predicates); j++)
	{
	  predicate_pt pd=(predicate_pt)array_get(ev->predicates, j);
	  int k;

	  for (k=0; k<array_count(pd->features); k++)
	    {
	      feature_pt ft=(feature_pt)array_get(pd->features, k);
	      if (!ft) { continue; }
	      corr[k]--;
	      pab[k]+=ft->alpha;
	    }
	}
      /* */      
      for (j=0; j<no_ocs; j++)
	{
	  if (corr[j]==m->max_pds) { continue; }
	  if (m->max_pds!=m->min_pds) { pab[j]+=m->cf_alpha*corr[j]; }
	  pab[j]=exp(pab[j]);
	  psum+=pab[j];
	}
      /* normalize */
      if (psum>0.0) { for (j=0; j<no_ocs; j++) { if (corr[j]!=m->max_pds) { pab[j]/=psum; } } }

      /* store success (could be done in loop above) */
      for (j=0; j<no_ocs && corr[j]==m->max_pds; j++) { /* nothing */ }
      b_pab=pab[j]; b_oc=j;
      for (j=j+1; j<no_ocs; j++)
	{ if (pab[j]>b_pab) { b_pab=pab[j]; b_oc=j; } }
      if (b_oc==ev->outcome) { pos+=ev->count; } else { neg+=ev->count; }
      
      /* update correction */
      if (m->max_pds!=m->min_pds)
	{ for (j=0; j<no_ocs; j++) { if (corr[j]!=m->max_pds) { cf_mod+=pab[j]*corr[j]*ev->count; } } }

      for (j=0; j<array_count(ev->predicates); j++)
	{
	  predicate_pt pd=(predicate_pt)array_get(ev->predicates, j);
	  int k;

	  for (k=0; k<array_count(pd->features); k++)
	    {
	      feature_pt ft=(feature_pt)array_get(pd->features, k);
	      if (!ft) { continue; }
 	      ft->mod+=pab[k]*ev->count; 
	    }
	}
    }

  for (i=0; i<array_count(m->predicates); i++)
    {
      predicate_pt pd=(predicate_pt)array_get(m->predicates, i);
      int j;

      for (j=0; j<array_count(pd->features); j++)
	{
	  feature_pt ft=(feature_pt)array_get(pd->features, j);
	  if (!ft) { continue; }
	  ft->alpha+=m->inv_max_pds*(ft->E - log(ft->mod));
	  ft->mod=0.0;
	}
    }
  
  /* update alpha for correction feature */
  if (cf_mod>0.0)
    { m->cf_alpha+=m->inv_max_pds*(m->cf_E - log(cf_mod)); }
  report(3, "done\r");

  return (double)pos/((double)(pos+neg));
}

/* ------------------------------------------------------------ */
void assign_probabilities2(model_pt m, array_pt pds, double p[])
{
  int no_ocs=array_count(m->outcomes);
  double inv_no_ocs=log(1.0/(double)no_ocs);
  int corr[no_ocs];
  double psum=0.0; 
  int j;

  for (j=0; j<no_ocs; j++) { p[j]=inv_no_ocs; corr[j]=0; }
  for (j=0; j<array_count(pds); j++)
    {
      predicate_pt pd=(predicate_pt)array_get(pds, j);
      int k;

      for (k=0; k<no_ocs; k++)
	{
	  feature_pt ft=(feature_pt)array_get(pd->features, k);

	  if (!ft) { continue; }
	  corr[k]++;
	  p[k]+=m->inv_max_pds*ft->alpha;
	}
    }
  for (j=0; j<no_ocs; j++)
    {
      p[j]=exp(p[j] + (1.0-corr[j]*m->inv_max_pds)*m->cf_alpha);
      psum+=p[j];
    }
  for (j=0; j<no_ocs; j++) { p[j]/=psum; }
}

/* ------------------------------------------------------------ */
void assign_probabilities(model_pt m, array_pt pds, double p[])
{
  int j;
  int no_ocs=array_count(m->outcomes);
  int corr[no_ocs];  
  double psum=0.0;
  
  for (j=0; j<no_ocs; j++) { p[j]=0.0; corr[j]=m->max_pds; }
  for (j=0; j<array_count(pds); j++)
    {
      predicate_pt pd=(predicate_pt)array_get(pds, j);
      int k;

      for (k=0; k<array_count(pd->features); k++)
	{
	  feature_pt ft=(feature_pt)array_get(pd->features, k);
	  if (!ft) { continue; }
	  corr[ft->outcome]--;
	  p[ft->outcome]+=ft->alpha;
	}
    }
  /* exponentiate and compute normalization */      
  for (j=0; j<no_ocs; j++)
    {
      /* if (m->max_pds!=m->min_pds) { p[j]+=m->cf_alpha*corr[j]; } */
      p[j]+=m->cf_alpha*corr[j];
      p[j]=exp(p[j]);
      psum+=p[j];
    }
  /* normalize */
  /* if (psum>0.0) { for (j=0; j<no_ocs; j++) { p[j]/=psum; } } */
  for (j=0; j<no_ocs; j++) { p[j]/=psum; }
}

/* ------------------------------------------------------------ */
void redistribute_probabilities(model_pt m, array_pt keep, double p[])
{
  double psum=0.0;
  double tmp[m->no_ocs];
  int i;
  
  if (!keep) { return; }
  memcpy(tmp, p, sizeof(double)*m->no_ocs);
  memset(p, 0, m->no_ocs*sizeof(double));

  /* 
  for (i=0; i<array_count(keep); i++)
    {
      int j=(int)array_get(keep, i);
      p[j]=tmp[j];
    }
  */

  for (i=0; i<array_count(keep); i++)
    {
      size_t j=(size_t)array_get(keep, i);
      psum+=tmp[j];      
    }
  for (i=0; i<array_count(keep); i++)
    {
      size_t j=(size_t)array_get(keep, i);
      p[j]=tmp[j]/psum;
    }
}

/* ------------------------------------------------------------ */
