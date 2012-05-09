/*
  Hashtable

  Copyright (C) 2001 Ingo Schröder
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

#ifndef HASH_H
#define HASH_H

/* ------------------------------------------------------------ */
struct hash_entry_s;
typedef struct hash_entry_s hash_entry_s, *hash_entry_pt;
struct hash_s;
typedef struct hash_s hash_s, *hash_pt;
struct hash_iterator_s;
typedef struct hash_iterator_s hash_iterator_s, *hash_iterator_pt;

/* ------------------------------------------------------------ */
/* creates a new hashtable with initial capacity and functions
   - capacity
   - loadfactor
   - hash function
   - equal function
*/
extern hash_pt hash_new(size_t, double, size_t (*)(void *), int (*)(void *, void *));

/* deletes a hashtable, but can't free memory for the content */
extern void hash_delete(hash_pt ht);

/* calls function `f' for each hash element */
extern void hash_map(hash_pt ht, void (*)(void *, void *));

/* like hash_map, but f is provided with generic data */
extern void hash_map1(hash_pt ht, void (*)(void *, void *, void *), void *);

/* like hash_map1, but f is provided with two arguments of generic data */
extern void hash_map2(hash_pt ht, void (*)(void *, void *, void *, void *), void *, void *);

/* like hash_map, but removes element from hash if f returns true */
extern void hash_filter(hash_pt ht, int (*)(void *, void *));

/* like hash_map, but frees hash, hash becomes inaccessible!! */
extern void hash_map_free(hash_pt ht, void (*)(void *, void *));

/* adds a key/value-pair to a hashtable */
extern void *hash_put(hash_pt ht, void *key, void *value);

/* retrieves a value for a key */
extern void *hash_get(hash_pt ht, void *key);

/* removes entry */
extern void *hash_remove(hash_pt ht, void *key);

/* clears all entries */
extern void hash_clear(hash_pt ht);

/* returns number of entries in hashtable */
extern size_t hash_size(hash_pt ht);

/* returns TRUE if empty */
extern int hash_is_empty(hash_pt ht);

/* returns TRUE if key is found */
extern int hash_contains_key(hash_pt ht, void *key);

/* returns TRUE if value is found, expensive! */
extern int hash_contains_value(hash_pt ht, void *value);

/* returns a new hash iterator object */
extern hash_iterator_pt hash_iterator_new(hash_pt ht);

/* returns the next key of a hash-iterator */
extern void *hash_iterator_next_key(hash_iterator_pt hi);

/* returns the next value of a hash-iterator */
extern void *hash_iterator_next_value(hash_iterator_pt hi);

/* frees hash iterator object */
extern void hash_iterator_delete(hash_iterator_pt hi);

/* returns product over all ASCII codes of string */
extern size_t hash_string_hash(void *s);

/* returns TRUE if s and t are the same strings */
extern int hash_string_equal(void *s, void *t);

/* ------------------------------------------------------------ */
#endif
