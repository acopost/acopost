/*
  Some utility functions
  
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

#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/* 0 errors only, 1 normal, ..., 6 really chatty */
extern int verbosity;

/* ------------------------------------------------------------ */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns number of CPU milliseconds used
*/
extern unsigned long used_ms(void);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   print information to STDERR and exit
*/
extern void error(char *format, ...);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   selectively print information to STDERR
   - mode: print only if abs(mode)<=verbosity
   - format: format string
   - ...: arguments
*/
extern void report(int mode, char *format, ...);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   print small spinning wheel
   - mode: print only if abs(mode)<=verbosity
*/
extern void print_progress(int mode);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   The intcmp() function return an integer less than, equal to, or greater than zero
   if the int pointed by ip is found, respectively, to be
   less than, to match, or be greater than the integer pointed by jp.
*/
int intcmp(const void *ip, const void *jp);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   tries to open file, exists on error
   - name: filename
   - mode: open mode
*/
extern FILE *try_to_open(const char *name, char *mode);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   successively returns tokens separated by one or more chars
   from sep string, not reentrant, temporarily modifies s,
   NOT reentrant
   typical use:
   for (t=tokenizer(b, " \t"); t; t=tokenizer(NULL, " \t"))
     { ... }
*/
char *tokenizer(char *s, const char *sep);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   reverses a string s, returns a pointer to buffer
*/
char *reverse(const char *s, char**buffer, size_t*n);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   This function is a replacement for POSIX.1-2008 getdelim()
*/
ssize_t readdelim(char **lineptr, size_t *n, int delim, FILE *stream);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   This function is a replacement for POSIX.1-2008 getline()
*/
ssize_t readline(char **lineptr, size_t *n, FILE *stream);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns substring of s with maximum length length,
   - if length>0: starting at pos
     substr("12345", 1, 3, &buffer, &size)="234"
   - if length<0: ending with pos 
     substr("12345", 3, -2, &buffer, &size)="34"
   returns a pointer to buffer
*/
char *substr(const char *s, size_t pos, ssize_t length, char**buffer, size_t *n);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns lowercase version of string s, returns a pointer
   to buffer
*/
char *lowercase(char *s, char **buffer, size_t *n);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns lowercase variant of c (including umlauts)
*/
char mytolower(char);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns the number of uppercase characters at the beginning of s
*/
size_t uppercase_prefix_length(char *s);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns the first uppercase character occurrence in s or NULL
*/
char* get_first_uppercase(char *s);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns non-zero if c is uppercase, 0 otherwise
*/
int is_uppercase(int c);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns length of common prefix
*/
size_t common_prefix_length(const char *, const char *);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns length of common suffix
*/
size_t common_suffix_length(const char *, const char *);

/* ------------------------------------------------------------ */
#endif
