/*
  Generalized Iterative Scaling
  
  Copyright (C) 2001 Ingo Schröder, ingo@nats.informatik.uni-hamburg.de

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

  int no_ocs;
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
