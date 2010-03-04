/*
  Prime numbers

  Copyright (C) 1997-2001 The DAWAI Team
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

#ifndef PRIMES_H
#define PRIMES_H

/* ------------------------------------------------------------ */
typedef unsigned long ulong;

/* ------------------------------------------------------------ */
/* rabin's probablistic primetest-algorithm
 * returns true if number is a prime by testing it a few times
 */
int primes_rabin (unsigned long no, unsigned long times);

/* returns the next prime after a given number */
unsigned long primes_next (unsigned long no, unsigned long times);

/* ------------------------------------------------------------ */
#endif
