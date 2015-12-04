/*
  Prime numbers

  Copyright (c) 2013, ACOPOST Developers Team
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

/*
  Original author: Giulio Paci <giuliopaci@gmail.com>

  Most of the theory behind these functions can be found on Wikipedia:
  http://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
  https://en.wikipedia.org/wiki/Modular_exponentiation

  The constant numbers in miller_rabin_size_t() were taken from
  http://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test#Deterministic_variants_of_the_test
  and
  http://miller-rabin.appspot.com/
*/

#include <stdlib.h>
#include "primes.h"

static inline size_t modular_addition_size_t_unsafe(size_t x, size_t y, size_t m)
{
	size_t z;
	z = m-x;
	if(z > y)
	{
		return x+y;
	}
	else
	{
		return y-z;
	}
}

static inline size_t modular_product_size_t_unsafe(size_t x, size_t y, size_t m)
{
	size_t z;
	if (x < y)
	{
		z = x;
		x = y;
		y = z;
	}
	z = 0;
	while( y > 0 )
	{
		if (y & 1)
		{
			z = modular_addition_size_t_unsafe(z, x, m);
		}
		y = y >> 1;
		x = modular_addition_size_t_unsafe(x, x, m);
	}
	return z;
}

static inline size_t modular_power_size_t_unsafe(size_t x, size_t y, size_t m)
{
	size_t z = 1;
	if( m <= 1 )
	{
		return 0;
	}
	while( y > 0 )
	{
		if (y & 1)
		{
			z =  modular_product_size_t_unsafe(z, x, m);
		}
		y = y >> 1;
		x = modular_product_size_t_unsafe(x,x,m);
	}
	return z;
}

size_t modular_addition_size_t(size_t x, size_t y, size_t m)
{
	return modular_addition_size_t_unsafe(x%m, y%m, m);
}

size_t modular_product_size_t(size_t x, size_t y, size_t m)
{
	return modular_product_size_t_unsafe(x%m, y%m, m);
}

size_t modular_power_size_t(size_t x, size_t y, size_t m)
{
	return modular_power_size_t_unsafe(x%m, y, m);
}


static inline int miller_rabin_pass_size_t(size_t x /* x>3, odd, prime to test */, size_t b /* b in [2, x-2] */)
{
	size_t y, z;
	int i;
	if( b < 2 )
	{
		return 1;
	}
	i = -1;
	y = x - 1;
	while ((y & 1) == 0)
	{
		y = y >> 1;
		i++;
	}
	z = modular_power_size_t_unsafe(b, y, x);
	y = x - 1;
	if ((z == 1) || (z == y))
	{
		return 1;
	}
	for(; i>0; i--)
	{
		z = modular_product_size_t_unsafe(z, z, x);
		if (z == 1)
		{
			return 0;
		}
		else if (z == y)
		{
			return 1;
		}
	}
	return 0;
}

static const size_t small_primes[] =
{
    3,
    5,
    7,
    11,
    13,
    17,
    19,
    23,
    29
};

int miller_rabin_size_t(size_t x)
{
	if(x<4)
	{
		if(x>1)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if( ! ( x & 1 ) )
	{
		return 0;
	}
	const size_t N = sizeof(small_primes) / sizeof(small_primes[0]);
	if( x > small_primes[N-1] )
	{
		int i;
		for (i = 0; i < N; ++i)
		{
			if( ! ( x % small_primes[i]) )
			{
				return 0;
			}
		}
	}
	if( x < 2047UL )
	{
		return 
			miller_rabin_pass_size_t(x, 2UL);
	}
	else if( x < 9080191UL )
	{
		return 
			miller_rabin_pass_size_t(x, 31UL)
			&& miller_rabin_pass_size_t(x, 73UL);
	}
	else if( sizeof(x) == 4 )
	{
		if( x < 4294967295UL /*MAX 32bit*/ )
		{
			return 
				miller_rabin_pass_size_t(x, 2UL)
				&& miller_rabin_pass_size_t(x, 7UL)
				&& miller_rabin_pass_size_t(x, 61UL);
		}
	}
	else if( x < 4759123141UL )
	{
		return 
			miller_rabin_pass_size_t(x, 2UL)
			&& miller_rabin_pass_size_t(x, 7UL)
			&& miller_rabin_pass_size_t(x, 61UL);
	}
	else if( x < 75792980677 )
	{
		return 
			miller_rabin_pass_size_t(x, 2UL)
			&& miller_rabin_pass_size_t(x, 379215UL)
			&& miller_rabin_pass_size_t(x, 457083754UL);
	}
        else if( x < 47636622961201UL )
	{
		return 
			miller_rabin_pass_size_t(x, 2UL)
			&& miller_rabin_pass_size_t(x, 2570940UL)
			&& miller_rabin_pass_size_t(x, 211991001UL)
			&& miller_rabin_pass_size_t(x, 3749873356UL);
	}
	else
	{
		return 
			miller_rabin_pass_size_t(x, 2UL)
			&& miller_rabin_pass_size_t(x, 325UL)
			&& miller_rabin_pass_size_t(x, 9375UL)
			&& miller_rabin_pass_size_t(x, 28178UL)
			&& miller_rabin_pass_size_t(x, 450775UL)
			&& miller_rabin_pass_size_t(x, 9780504UL)
			&& miller_rabin_pass_size_t(x, 1795265022UL);
	}
}

size_t miller_rabin_next_prime_size_t(size_t n)
{
	if(n<2)
	{
		return 2;
	}
	n |= 1UL;
	while (!miller_rabin_size_t(n))
	{
		if(n==~0UL)
		{
			return 0;
		}
		n+=2;
	}
	return n;
}
