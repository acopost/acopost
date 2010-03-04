/*
  One-dimensional Arrays

  Copyright (C) 2001 Ingo Schröder
            (C) 2010 Tiago Tresoldi

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

#ifndef ARRAY_H
#define ARRAY_H
/* ------------------------------------------------------------ */
typedef struct array_s
{
  int size;
  int count;
  void **v;
} array_t;
typedef array_t *array_pt;

/* ------------------------------------------------------------ */
array_pt array_new(unsigned int);
void array_fill(array_pt, void *);
array_pt array_new_fill(unsigned int, void *);
void array_free(array_pt);
void array_clear(array_pt);
array_pt array_clone(array_pt);
unsigned int array_add(array_pt, void *);
unsigned int array_add_unique(array_pt, void *);
void array_delete_item(array_pt, void *);
void array_delete_duplicates(array_pt);
void *array_delete_index(array_pt, unsigned int);
void array_trim(array_pt);
void array_filter_with(array_pt, int (*)(void *, void *), void *);
void array_filter(array_pt, int (*)(void *));
void array_map(array_pt, void (*)(void *));
void array_map1(array_pt, void (*)(void *, void *), void *);
void array_map2(array_pt, void (*)(void *, void *, void *), void *, void *);
#define array_map_with(a, b, c) array_map1(a, b, c)

int array_size(array_pt a);
int array_count(array_pt a);
void *array_get(array_pt a, int i);

void *array_set(array_pt, int, void *);

/* ------------------------------------------------------------ */
#endif
