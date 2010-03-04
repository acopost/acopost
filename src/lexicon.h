/*
  Lexicon data structure

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

#ifndef LEXICON_H
#define LEXICON_H

/* ------------------------------------------------------------ */
#include "array.h"
#include "hash.h"

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
typedef struct lexicon_s
{
  char *fname;        /* filename */
  array_pt tags;      /* tags */
  hash_pt taghash;    /* lookup table tags[taghash{"tag"}-1]="tag" */
  int defaulttag;     /* */
  int *sorter;        /* */
  int *tagcount;      /* */
  hash_pt words;      /* dictionary: string->int (best tag) */
  void *userdata;     /* */
} lexicon_t;
typedef lexicon_t *lexicon_pt;

typedef struct word_s
{
  char *string;      /* grapheme */
  int count;         /* total number of occurances */
  int defaulttag;    /* most frequent tag index */
  int *sorter;       /* tagcount[sorter[i]]>=tagcount[sorter[i+1]] */
  int *tagcount;     /* maps tag index -> no. of occurances */
  void *userdata;    /* generic pointer for user data */
} word_t;
typedef word_t *word_pt;

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------
   returns name of tag index i in lexicon
*/
extern char *tagname(lexicon_pt l, int i);

/* ------------------------------------------------------------
   returns index of tag t in lexicon or -1
*/
extern int find_tag(lexicon_pt l, char *t);

/* ------------------------------------------------------------
   reads lexicon file fn and returns new lexicon
   (if fn==NULL it uses STDIN)
*/
extern lexicon_pt read_lexicon_file(char *fn);

/* ------------------------------------------------------------ */
#endif
