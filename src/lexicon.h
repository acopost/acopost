/*
  Lexicon data structure

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
