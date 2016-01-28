/*
  Generalized Iterative Scaling

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

#ifndef GIS_H
#define GIS_H

/* ------------------------------------------------------------ */
#include "array.h"

/* ------------------------------------------------------------ */
typedef struct event_s
{
  int count;
  int outcome;
  array_pt predicates;
} event_t;
typedef event_t *event_pt;

typedef struct predicate_s
{
  int count;
  array_pt features;
  void *data;
} predicate_t;
typedef predicate_t *predicate_pt;

typedef struct feature_s
{
  int count;
  int outcome;
  predicate_pt predicate;
  double E;                     /* empirical expectation value: Ep~ */
  double alpha;                 /* parameter: \alpha^{(n)} */
  double mod;                   /* temp. value: addititive component */
} feature_t;
typedef feature_t *feature_pt;

typedef struct model_s
{
  int no_fts;

  size_t no_ocs;
  array_pt outcomes;

  int min_pds;
  int max_pds;
  double inv_max_pds;
  array_pt predicates;
  
  double cf_alpha;
  double cf_E;

  void *userdata;
} model_t;
typedef model_t *model_pt;


/* ------------------------------------------------------------ */
/* parameters: count, outcome */
event_pt new_event(int, int);

/* parameters: count, data, no_ocs */
predicate_pt new_predicate(int, void *, int);
void delete_predicate(predicate_pt pd);

/* parameters: count, outcome, predicate */
feature_pt new_feature(int, int, predicate_pt);
void delete_feature(feature_pt);

/* */
model_pt new_model(array_pt evs, array_pt pds);

/* ------------------------------------------------------------
  accuracy of all events in evs 
  - parameters: model, events
*/
/* double accuracy(model_pt, array_pt); */

/* ------------------------------------------------------------
   eval predicates in array, return probabilities in p
   -parameters: model, predicates, p field
*/
void assign_probabilities(model_pt, array_pt, double []);
void assign_probabilities2(model_pt, array_pt, double []);

/* ------------------------------------------------------------
   GIS training
   - m: model to train
   - mi: maximum number of iteration
   - th: threshold for delta
*/
/* void train_model(model_pt m, int mi, double th); */

/* ------------------------------------------------------------
  one iteration of GIS, returns accuracy on training set
  - parameters: model, events
*/
double train_iteration(model_pt, array_pt);

/* ------------------------------------------------------------
   redistribute the probability in p setting all outcomes except
   those in keep to zero
*/
void redistribute_probabilities(model_pt m, array_pt keep, double p[]);

/* ------------------------------------------------------------ */
#endif
