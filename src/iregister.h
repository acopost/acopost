/*
  Lexicon data structure

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

#ifndef IREGISTER_H
#define IREGISTER_H

#include <stddef.h> /* for ptrdiff_t and size_t. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* ------------------------------------------------------------ */
struct iregister_s;
typedef struct iregister_s iregister_s, *iregister_pt;

/* ------------------------------------------------------------ */
/* creates a new name-index register with initial capacity
   - capacity
*/
iregister_pt iregister_new(size_t cp);

/* stores and retrieves a shared string equivalent to s */
const char *iregister_get(iregister_pt st, const char *s);

/* clears all entries, stored strings will be invalid */
void iregister_clear(iregister_pt st);

/* deletes a string register, stored strings will also be invalid  */
void iregister_delete(iregister_pt st);

/* ------------------------------------------------------------ */
const char *iregister_get_name(iregister_pt l, int i);

ptrdiff_t iregister_get_index(iregister_pt l, const char *t);

size_t iregister_get_length(iregister_pt l);

ptrdiff_t  iregister_add_name(iregister_pt l, const char* s);

size_t iregister_add_unregistered_name(iregister_pt l, const char* s);

void iregister_delete(iregister_pt l);

/* ------------------------------------------------------------ */
#endif
