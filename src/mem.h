/*
  Memory management

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

#ifndef MEM_H
#define MEM_H
void mem_free (void *);
/*void *mem_malloc (unsigned int); by tiago */
void *mem_malloc (size_t);
void *mem_realloc (void *, unsigned int);
#endif
