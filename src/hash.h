/*
  Hashtable

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

/* like hash_map1, but f is provided with two arguments of generic data */
extern void hash_map3(hash_pt ht, void (*)(void *, void *, void *, void *, void *), void *, void *, void *);

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
