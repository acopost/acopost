/*
  Prime number test

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

/* ------------------------------------------------------------ */
#include "config-common.h"
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include "primes.h"

/* ------------------------------------------------------------ */
/*typedef unsigned long ulong;*/

/* ----------------------------------------------------------------------
 * addition in a modulo
 */
static ulong add_mod (ulong x, ulong y, ulong m)
{
  return ((x + y) % m);
}

/* ----------------------------------------------------------------------
 * multiplication in a modulo
 */
static ulong mult_mod (ulong x, ulong y, ulong m)
{
  ulong sum;

  if (x < y)
    return (mult_mod(y, x, m));

  for (sum = 0; y; y = y >> 1, x = add_mod(x, x, m))
    if (y & 1)
      sum = add_mod(sum, x, m);

  return sum;
}

/* ----------------------------------------------------------------------
 * exponent in a modulo
 */
static ulong pow_mod (ulong x, ulong y, ulong m)
{
  ulong prod;

  for (prod = 1; y; y = y >> 1, x = mult_mod(x, x, m))
    if (y & 1)
      prod = mult_mod(prod, x, m);

  return prod;
}

/* ----------------------------------------------------------------------
 * rabin's probablistic primetest-algorithm
 * returns true if number is a prime by testing it a few times
 */
int primes_rabin (ulong n, ulong t)
{
  ulong i;
  int flag;
  double x;

  for (i = 0, flag = 1; flag && i < t; i++) {
    x = floor(drand48() * (n - 1)) + 1;
    flag = (pow_mod((ulong)x, n - 1, n) == 1);
  }

  return flag;
}

/* ----------------------------------------------------------------------
 * returns the next prime after a given number
 */
ulong primes_next (ulong number, ulong times)
{
  srand48(time(NULL));

  while (!primes_rabin(number, times))
    number++;
  return number;
}

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

