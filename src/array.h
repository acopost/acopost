/*
  One-dimensional Arrays

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

#ifndef ARRAY_H
#define ARRAY_H
/* ------------------------------------------------------------ */
typedef struct array_s
{
  size_t size;
  size_t count;
  void **v;
} array_t;
typedef array_t *array_pt;

/* ------------------------------------------------------------ */
array_pt array_new(size_t);
void array_fill(array_pt, void *);
array_pt array_new_fill(size_t, void *);
void array_free(array_pt);
void array_clear(array_pt);
array_pt array_clone(array_pt);
size_t array_add(array_pt, void *);
size_t array_add_unique(array_pt, void *);
void array_delete_item(array_pt, void *);
void array_delete_duplicates(array_pt);
void *array_delete_index(array_pt, size_t);
void array_trim(array_pt);
void array_filter_with(array_pt, int (*)(void *, void *), void *);
void array_filter(array_pt, int (*)(void *));
void array_map(array_pt, void (*)(void *));
void array_map1(array_pt, void (*)(void *, void *), void *);
void array_map2(array_pt, void (*)(void *, void *, void *), void *, void *);
#define array_map_with(a, b, c) array_map1(a, b, c)

size_t array_size(array_pt a);
size_t array_count(array_pt a);
void *array_get(array_pt a, size_t i);

void *array_set(array_pt, size_t, void *);

/* ------------------------------------------------------------ */
#endif
