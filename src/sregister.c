/*
  String Register

  Copyright (c) 2001-2002, Ingo Schröder
  Copyright (c) 2007-2015, ACOPOST Developers Team
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

#include "config-common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdarg.h>
#include <errno.h>

#include "mem.h"
#include "hash.h"
#include "sregister.h"

struct sregister_s {
  hash_pt g_table;
  size_t cp;
};

static void _free_g_table_entry(void *key, void *value)
{
  /* Don't free key; it points to the same memory as value. */
  /* Don't use mem_free; the memory was obtained with strdup,
   * in register_string. 
   */
  free(value);
}

/* ------------------------------------------------------------ */
char *sregister_get(sregister_pt st, char *s)
{
  char *t=NULL;

  if (!s) { return s; } 
  if (!st->g_table) 
    { st->g_table=hash_new(st->cp, 0.6, hash_string_hash, hash_string_equal); }
  else { t=hash_get(st->g_table, s); if (t) { return t; } }

  t=strdup(s);
  hash_put(st->g_table, t, t);
  return t;
}
	
/* ------------------------------------------------------------ */
void sregister_clear(sregister_pt st)
{
  if (st->g_table) {
    hash_map(st->g_table, _free_g_table_entry);
  }
  hash_clear(st->g_table);
}
	
/* ------------------------------------------------------------ */
void sregister_delete(sregister_pt st)
{
  sregister_clear(st);
  hash_delete(st->g_table);
  mem_free(st);
}

/* ---------------------------------------------------------------------- */
/* ----------------------------------------------------------------------
 * creates and returns a new hashtable
 * cp - initial capacity
 */
sregister_pt sregister_new (size_t cp)
{
  sregister_pt st;
  /* creates the string register */
  st = (sregister_pt)mem_malloc(sizeof(sregister_s));
  if(st != NULL)
    st->g_table = NULL;
  st->cp = cp;
  return(st);
}
