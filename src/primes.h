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

#ifndef PRIMES_H
#define PRIMES_H

#include <stdlib.h>

/**
 * Evaluate (x+y)%m.
 *
 * The result is guaranteed to be correct for every possible input
 * values.
 */
size_t modular_addition_size_t(size_t x, size_t y, size_t m);

/**
 * Evaluate (x*y)%m.
 *
 * The result is guaranteed to be correct for every possible input
 * values.
 */
size_t modular_product_size_t(size_t x, size_t y, size_t m);

/**
 * Evaluate (x^y)%m.
 *
 * The result is guaranteed to be correct for every possible input
 * values.
 */
size_t modular_power_size_t(size_t x, size_t y, size_t m);

/**
 * Perform deterministic variant of miller-rabin test.
 *
 * The result is guaranteed to be correct for every possible input
 * values up to 2^64.
 *
 * @return 1 if x is prime, 0 if x is composite
 */
int miller_rabin_size_t(size_t x);

/**
 * Given an input number, output the first prime number greater than
 * or equal to the input.
 *
 * @return the first prime number greater than or equal to x, or 0 if
 * no such number exist within the size_t range (i.e., if an overflow
 * happens).
 */
size_t miller_rabin_next_prime_size_t(size_t x);

#endif
