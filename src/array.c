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

/* ------------------------------------------------------------ */
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "mem.h"

/* ------------------------------------------------------------ */
/* creates and returns a new array                              */
array_pt array_new (size_t size)
{
  array_pt arr = (array_pt)mem_malloc(sizeof(array_t));

  arr->size = size;
  arr->count = 0;
  arr->v = (void **)mem_malloc(size*sizeof(void *));
  memset(arr->v, 0, sizeof(void *)*size);

  return arr;
}

/* ------------------------------------------------------------ */
/* fills an array with a given value                            */
void array_fill (array_pt arr, void *p)
{
  size_t i;

  for (i=0; i < arr->size; i++)
    arr->v[i] = p;
}

/* ------------------------------------------------------------ */
/* creates and returns a new array filled with a given value    */
array_pt array_new_fill (size_t size, void *p)
{
  array_pt arr = array_new(size);

  arr->count = size;
  array_fill(arr, p);

  return arr;
}

/* ------------------------------------------------------------ */
/* frees an array and its contents                              */
void array_free (array_pt arr)
{
  mem_free(arr->v);
  mem_free(arr);
}

/* ------------------------------------------------------------ */
/* clears an array, without freeing (much faster)               */
void array_clear (array_pt arr)
{
  arr->count = 0;
}

/* ------------------------------------------------------------ */
/* returns a copy of a given array                              */
array_pt array_clone (array_pt arr)
{
  array_pt arr_clone = array_new(arr->size);

  arr_clone->size = arr->size;
  arr_clone->count = arr->count;
  /* NOT necessary, since we already did that in array_new.
    arr_clone->v = (void **)mem_malloc(arr->size*sizeof(void *));
  */
  (void)memcpy(arr_clone->v, arr->v, arr_clone->size*sizeof(void *));

  return arr_clone;
}

/* ------------------------------------------------------------ */
/* adds a given value to an array                               */
size_t array_add (array_pt arr, void *p)
{
  /* TODO: allocation starts at 8 and then goes *2, perhaps it would be
   * better to use a sequence fibonacci-like or prime-like */
  if (arr->size <= arr->count)
  {
    /* set new size */
    if (arr->size <= 0)
      arr->size = 8;
    else
      arr->size *= 2;

    /* allocate the needed space */
    arr->v = (void **)mem_realloc(arr->v, arr->size*sizeof(void *));
  }

  arr->v[arr->count] = p;

  return arr->count++;
}

/* ------------------------------------------------------------ */
/* adds a given value to an array only if the value is not
   already present                                              */
size_t array_add_unique (array_pt arr, void *p)
{
  size_t k;

  for (k = 0; k < arr->count; k++)
    if (arr->v[k] == p)
      return k;

  return array_add(arr, p);
}

/* ------------------------------------------------------------ */
/* deletes all occurences of a value from an array              */
void array_delete_item (array_pt arr, void *p)
{
  size_t i, j, oldcount = arr->count;

  for (i = 0, j = 0; i < oldcount; i++)
    {
      if (arr->v[i] == p)
        arr->count--;
      else {
        arr->v[j] = arr->v[i];
        j++;
      }
    }
}

/* ------------------------------------------------------------ */
/* deletes all duplicates from an array                         */
/* TODO: guarantee that it is safe */
void array_delete_duplicates (array_pt arr)
{
  size_t i, j, k, oldcount;

  for (k = 0; k < arr->count; k++)
    {
      oldcount = arr->count;
      for (i = k+1, j = i; i < oldcount; i++)
	{
	  if (arr->v[i] == arr->v[k])
            arr->count--;
	  else {
            arr->v[j] = arr->v[i];
            j++;
          }
	}
    }
}

/* ------------------------------------------------------------ */
/* deletes the element at a given index of the array, shifting
 * the following elements to the left */
/* TODO: guarantee that it is safe */
void *array_delete_index (array_pt arr, size_t idx)
{
  void *p = arr->v[idx];
  size_t i;

  for (i = idx+1; i < arr->count; i++)
    arr->v[i-1] = arr->v[i];
  arr->count--;

  return p;
}

/* ------------------------------------------------------------ */
/* the 'size' of the array will likely be bigger than its
  'count' many times; this function trims it */
/* TODO: analyse if it should be automatically called in some
         occasions */
void array_trim (array_pt arr)
{
  if (arr->size > arr->count)
    {
      arr->size = arr->count;
      arr->v = (void **)mem_realloc(arr->v, arr->size*sizeof(void *));
    }
}

/* ------------------------------------------------------------ */
void array_filter_with (array_pt arr, int (*func)(void *, void *), void *data)
{
  size_t i, j;
  size_t oldcount = arr->count;

  for (i = 0, j = 0; i < oldcount; i++)
    {
      if ( func(arr->v[i], data) )
        arr->count--;
      else {
        arr->v[j] = arr->v[i];
        j++;
      }
    }
}

/* ------------------------------------------------------------ */
void array_filter (array_pt arr, int (*func)(void *))
{
  size_t i, j;
  size_t oldcount = arr->count;

  for (i  =0, j = 0; i < oldcount; i++)
    {
      if ( func(arr->v[i]) )
        arr->count--;
      else {
        arr->v[j] = arr->v[i];
        j++;
      }
    }
}

/* ------------------------------------------------------------ */
void array_map (array_pt arr, void (*func)(void *))
{
  size_t i;

  for (i = 0; i < arr->count; i++)
    func(arr->v[i]);
}

/* ------------------------------------------------------------ */
void array_map1 (array_pt arr, void (*func)(void *, void *), void *p)
{
  size_t i;

  for (i = 0; i < arr->count; i++)
    func(arr->v[i], p);
}

/* ------------------------------------------------------------ */
void array_map2 (array_pt arr, void (*func)(void *, void *, void *), void *p1, void *p2)
{
  size_t i;

  for (i = 0; i < arr->count; i++)
    func(arr->v[i], p1, p2);
}

/* ------------------------------------------------------------ */
void *array_set (array_pt arr, size_t i, void *p)
{
  void *old;

  if (arr->size <= i)
    {
      if (arr->size <= 0)
        arr->size = 8;
      while (arr->size <= i)
        arr->size *= 2;
      arr->v = (void **)mem_realloc(arr->v, arr->size*sizeof(void *));
      memset(&arr->v[arr->count], 0, sizeof(void *)*(arr->size - arr->count));
    }
  old = arr->v[i];
  arr->v[i] = p;

  if (arr->count <= i)
    arr->count=i+1;

  return old;
}

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

/* returns the size of the array */
/* TODO: remove function? */
size_t array_size (array_pt arr)
{
    return arr->size;
}

/* returns the count of the array */
/* TODO: remove function? */
size_t array_count (array_pt arr)
{
    return arr->count;
}

/* returns element 'i' from the array */
/* TODO: could be made safe by checking if it is in limits,
   otherwise the programmer can access directly the array */
void *array_get (array_pt arr, size_t i)
{
    return arr->v[i];
}
