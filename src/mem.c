/*
  Memory management

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

/* ------------------------------------------------------------ */
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"
#include "util.h"

/* ------------------------------------------------------------ */
void mem_free (void *p)
{
  free(p);
}

/* ------------------------------------------------------------ */
/*void *mem_malloc (unsigned int size) by tiago */
void *mem_malloc (size_t size)
{
  void *p = malloc(size);
  if (!p)
      error("memory allocation failed (no memory left?) [mem_malloc]\n");

  return p;
}

/* ------------------------------------------------------------ */
void *mem_realloc (void *p, unsigned int size)
{
  p = realloc(p, size);
  if (!p)
      error("memory allocation failed (no memory left?) [mem_realloc]\n");

  return p;
}
