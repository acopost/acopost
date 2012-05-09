/*
  Hashtables

  Copyright (C) 2001, 2002 Ingo Schröder
                2010 Tiago Tresoldi

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
#include <stdio.h>
#include <string.h>
#include "hash.h"
#include "primes.h"
#include "mem.h"

/* ------------------------------------------------------------ */
#define PRIME_TESTS 10
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ------------------------------------------------------------ */
struct hash_entry_s {
  void *key;                       /* hash key */
  void *value;                     /* value */
  struct hash_entry_s *next;       /* pointer to next entry */
};

struct hash_s {
  size_t count;                    /* number of entries in hashtable */
  size_t threshold;                /* limit when table is rehashed */
  size_t capacity;                 /* capacity of table */
  double loadfactor;               /* ratio when to rehash */
  int (*equal)(void *, void *);    /* equality function for key */
  size_t (*hash)(void *);          /* hash function */
  hash_entry_pt *entries;          /* table of entries */
};

struct hash_iterator_s {
  hash_pt hashtable;               /* where do we belong to? */
  size_t index;                    /* index of ``entry'' */
  hash_entry_pt entry;             /* determines _next_ hash_entry_pt */
};

/* ----------------------------------------------------------------------
 * rehash hashtable, double capacity
 */
void rehash(hash_pt ht)
{
  hash_entry_pt *newEntries;
  hash_entry_pt entry, old;
  size_t i, index;
  size_t newCapacity = primes_next(ht->capacity * 2, PRIME_TESTS);

#ifdef DEBUG
  printf("DEBUG: rehashhash_pt %d --> %d...", ht->capacity, newCapacity);
#endif
  newEntries = 
    (hash_entry_pt *)mem_malloc(sizeof(hash_entry_s) * newCapacity);
  for (i = 0; i < newCapacity; i++)
    newEntries[i] = (hash_entry_pt)NULL;
  for (i = 0; i < ht->capacity; i++)
    for (old = ht->entries[i]; old != NULL; ) {
      entry = old;
      old = old->next;

      index = (ht->hash(entry->key) & 0x7fffffff) % newCapacity;
      
      entry->next = newEntries[index];
      newEntries[index] = entry;
    }

  ht->threshold = (newCapacity * ht->loadfactor);
  ht->capacity = newCapacity;
  mem_free(ht->entries);
  ht->entries = newEntries;

#ifdef DEBUG
  printf("done\n");
#endif
}

/* ---------------------------------------------------------------------- */
/* ----------------------------------------------------------------------
 * creates and returns a new hashtable
 * cp - initial capacity
 * lf - load factor (ratio when to rehash)
 */
hash_pt hash_new (size_t cp, double lf, size_t (*hash)(void *), int (*equal)(void *, void *))
{
  hash_pt ht;
  size_t i;

  /* guarantees a minimum capacity of 3 -- TODO: why? */
  if (cp < 3) cp = 3;

  /* creates the hash table */
  ht = (hash_pt)mem_malloc(sizeof(hash_s));
  ht->count = 0;
  ht->capacity = primes_next(cp, PRIME_TESTS);
  ht->threshold = (ht->capacity * lf);
  ht->loadfactor = lf;
  ht->equal = equal;
  ht->hash = hash;
  ht->entries = (hash_entry_pt *)mem_malloc(sizeof(hash_entry_s)*ht->capacity);

  /* initializes all entries to NULL -- TODO: why? */
  for (i = 0; i < ht->capacity; i++)
    ht->entries[i] = NULL;

  return(ht);
}

/* ----------------------------------------------------------------------
 * adds a new key/value pair to the hashtable
 * rehashes the hashtable if necessary
 */
void *hash_put (hash_pt ht, void *key, void *value)
{
  hash_entry_pt entry;
  size_t index;
  void *p;

  if (ht == NULL || key == NULL || value == NULL) {
    fprintf(stderr, "ERROR: [hash_put] arguments must not be NULL.\n");
    abort();
  }

  index = (((*ht->hash)(key)) & 0x7fffffff) % ht->capacity;
  for (entry = ht->entries[index]; entry != NULL; entry = entry->next) {
    if ((*ht->equal)(entry->key, key)) {
      p = entry->value;
      entry->key = key;
      entry->value = value;
      return p;
    }
  }

  if (ht->count >= ht->threshold) {
    /* The number of entries exceeds the thresholds. Rehash the table.
     * TODO:
     * - touching fresh buckets is counted and punished by rehashing
     * - rehashing should depend on collisions not on tabelsize
     */
    rehash(ht);
    return hash_put(ht, key, value);
  }

  entry = (hash_entry_pt)mem_malloc(sizeof(hash_entry_s));
  entry->key = key;
  entry->value = value;
  entry->next = ht->entries[index];
  ht->entries[index] = entry;
  ht->count++;

  return NULL;
}


/* ----------------------------------------------------------------------
 * retrieve value associated with the key
 */
void *hash_get (hash_pt ht, void *key)
{
  hash_entry_pt entry;
  size_t index;

  if (!ht || !key)
    {
      fprintf(stderr, "ERROR: hash_get: arguments must not be NULL\n");
      abort();
    }

  index = (((*ht->hash)(key)) & 0x7fffffff) % ht->capacity;
  for (entry = ht->entries[index]; entry != NULL; entry = entry->next)
    if ( (*ht->equal)(entry->key, key) )
      return entry->value;

  return NULL;
}

/* ----------------------------------------------------------------------
 * remove key/value pair from hashtable
 */
void *hash_remove(hash_pt ht, void *key)
{
  hash_entry_pt entry, prev;
  size_t index;
  void *value;

  index=(((*ht->hash)(key)) & 0x7fffffff) % ht->capacity;
  for (entry=ht->entries[index], prev=NULL; entry; prev=entry, entry=entry->next)
    {
      if ((*ht->equal)(entry->key, key))
	{
	  if (prev) { prev->next=entry->next; }
	  else { ht->entries[index]=entry->next; }
	  ht->count--;
	  value=entry->value;
	  mem_free(entry);
	  return value;
	}
    }
  return NULL;
}

/* ----------------------------------------------------------------------
 * clears all entries
 */
void hash_clear(hash_pt ht)
{
  size_t i;

  for (i=0; i<ht->capacity; i++)
    {
      hash_entry_pt e, f;
      for (e=ht->entries[i]; e; e=f) { f=e->next; mem_free(e); }
      ht->entries[i]=NULL;
    }
  ht->count=0;
}

/* ----------------------------------------------------------------------
 * returns number of entries in hashtable
 */
size_t hash_size(hash_pt ht)
{
  if (!ht)
    {
      fprintf(stderr, "ERROR: hash_size: argument must not be NULL\n");
      abort();
    }

  return(ht->count);
}

/* ----------------------------------------------------------------------
 * returns TRUE if hashtable is empty
 */
int hash_is_empty(hash_pt ht)
{
  if (ht == NULL) {
    fprintf(stderr, "ERROR: hash_is_empty: argument must not be NULL\n");
    abort();
  }

  return(ht->count == 0 ? TRUE : FALSE);
}

/* ----------------------------------------------------------------------
 * returns TRUE if hashtable contains key
 */
int hash_contains_key(hash_pt ht, void *key)
{
  hash_entry_pt entry;
  size_t index;

  if (ht == NULL || key == NULL) {
    fprintf(stderr, "ERROR: hash_constains_key: arguments must not be NULL\n");
    abort();
  }

  index = (((*ht->hash)(key)) & 0x7fffffff) % ht->capacity;
  for (entry = ht->entries[index]; entry != NULL; entry = entry->next)
    if ((*ht->equal)(entry->key, key))
      return(TRUE);
  
  return(FALSE);
}

/* ----------------------------------------------------------------------
 * returns TRUE if hashtable contains value, expensive!
 */
int hash_contains_value(hash_pt ht, void *value) 
{
  hash_entry_pt entry;
  size_t i;

  if (ht == NULL || value == NULL) {
    fprintf(stderr, "ERROR: hash_constains_value: arguments must not be NULL\n");
    abort();
  }
  
  for (i = 0; i < ht->count; i++)
    for (entry = ht->entries[i]; entry != NULL; entry = entry->next)
      if (value == entry->value)
	return(TRUE);
  
  return(FALSE);
}

/* ----------------------------------------------------------------------
 * deletes hashtable, but can't free memory for the content
 */
void hash_delete(hash_pt ht)
{
  hash_clear(ht);
  mem_free(ht->entries);
  mem_free(ht);
}

/* ----------------------------------------------------------------------
 * calls function `f(key, value)' for each hash element
 */
void hash_map(hash_pt ht, void (*f)(void *, void *))
{
   hash_entry_pt entry;
   size_t i;

   for (i = 0; i < ht->capacity; i++) {
     for (entry = ht->entries[i]; entry != NULL; entry = entry->next) {
       (*f)(entry->key, entry->value);
     }
   }
}

/* ----------------------------------------------------------------------
 * like hash_map, but f is provided with generic data 
 */
void hash_map1(hash_pt ht, void (*f)(void *, void *, void*), void *d)
{
   hash_entry_pt entry;
   size_t i;

   for (i = 0; i < ht->capacity; i++) {
     for (entry = ht->entries[i]; entry != NULL; entry = entry->next) {
       (*f)(entry->key, entry->value, d);
     }
   }
}

/* ----------------------------------------------------------------------
 * like hash_map1, but f is provided with two arguments 
 */
void hash_map2(hash_pt ht, void (*f)(void *, void *, void*, void*), void *d1, void *d2)
{
   hash_entry_pt entry;
   size_t i;

   for (i = 0; i < ht->capacity; i++) {
     for (entry = ht->entries[i]; entry != NULL; entry = entry->next) {
       (*f)(entry->key, entry->value, d1, d2);
     }
   }
}

/* ----------------------------------------------------------------------
 * like hash_map, but removes element from hash if f returns true
 */
void hash_filter(hash_pt ht, int (*f)(void *, void *))
{
  hash_entry_pt he, prev, next;
  size_t i;
  
  for (i=0; i<ht->capacity; i++)
    {
      for (he=ht->entries[i], prev=NULL; he; prev=he, he=next)
	{
	  next=he->next;
	  if (f(he->key, he->value))
	    {
	      if (prev) { prev->next=next; }
	      else { ht->entries[i]=next; }
	      mem_free(he);
	      ht->count--;
	    }
	}
    }
}

/* ----------------------------------------------------------------------
 * like hash_map, but frees hashtable, ht becomes inaccessible
 */
void hash_map_free(hash_pt ht, void (*f)(void *, void *))
{
   hash_entry_pt entry, next;
   size_t i;

   for (i = 0; i < ht->capacity; i++) {
      for (entry = ht->entries[i]; entry != NULL; entry = next) {
	 next = entry->next;
	 (*f)(entry->key, entry->value);
	 mem_free(entry);
      }
   }
   mem_free(ht->entries);
   mem_free(ht);
}

/* ----------------------------------------------------------------------
 * returns a new hash iterator object 
 *
 *  hi = hash_iterator_new(ht);
 *  while (NULL != (key = hash_iterator_next_key(hi))) {
 *    do something with key;
 *  }
 *  hash_iterator_delete(hi);
 *
 */
hash_iterator_pt hash_iterator_new(hash_pt ht)
{
  hash_iterator_pt hi;

  /* Some checks would be nice but I don't have the time right now. */
  hi = (hash_iterator_pt)mem_malloc(sizeof(hash_iterator_s));
  hi->hashtable = ht;
  hi->index = -1;
  hi->entry = NULL;
  while (!hi->entry && ++hi->index < ht->capacity) {
    hi->entry=ht->entries[hi->index];
  }

  return(hi);
}

/* ----------------------------------------------------------------------
 * returns the next key of a hash-iterator, 
 * iterator points to the following entry afterwards
 */
void *hash_iterator_next_key(hash_iterator_pt hi)
{
  hash_entry_pt current=hi->entry;
  hash_pt ht=hi->hashtable;

  if (!current) {
    return(NULL);
  }

  hi->entry = hi->entry->next;
  while (!hi->entry && ++hi->index < ht->capacity) { 
    hi->entry=ht->entries[hi->index];
  }

  return(current->key);
}

/* ----------------------------------------------------------------------
 * returns the next value of a hash-iterator, 
 * iterator points to the following entry afterwards
 */
void *hash_iterator_next_value(hash_iterator_pt hi)
{
  hash_entry_pt current;
  hash_pt ht = hi->hashtable;

  if (NULL == (current = hi->entry)) {
    return(NULL);
  }

  if (NULL == (hi->entry = hi->entry->next)) {
    while (++hi->index < ht->capacity) { 
      /*   for (hi->index++; hi->index < ht->capacity; hi->index++) { */
      if (NULL != (hi->entry = ht->entries[hi->index])) {
	return(current->value);
      }
    }
  }

  return(current->value);
}

/* ----------------------------------------------------------------------
 * frees hash iterator object 
 */
void hash_iterator_delete(hash_iterator_pt hi)
{
  /* This is easy :-) */
  mem_free(hi);
}

/* ----------------------------------------------------------------------
 * hash_string_hash
 * 
 * TODO: 
 * - try different hash functions
 * - strlen should go out -> supply length of key
 */
size_t hash_string_hash(void *p)
{
  register char *s=(char *)p;
  register size_t l=strlen(s);
  register size_t v;

  /* rotating ala knuth */
  v=l;
  while(l) { v=(v<<5)^(v>>27)^s[--l]; }
  return v;
}

/* ----------------------------------------------------------------------
 * hash_string_equal
 */
int hash_string_equal(void *s, void *t)
{
/*   fprintf(stderr, "hash_string_equal: ->%s<- vs. ->%s<- == %d\n", s, t, strcmp((char *)s, (char *)t));  */
  return !strcmp((char *)s, (char *)t);
}

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
