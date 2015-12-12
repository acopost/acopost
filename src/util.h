/*
  Some utility functions
  
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

#ifndef UTIL_H
#define UTIL_H

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
extern FILE *try_to_open(char *name, char *mode);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns pointer to single internal static buffer that
   holds the basename of the filename name minus the suffix
   (if specified)
   not reentrant
*/
char *acopost_basename(char *name, char *suffix);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   successively returns tokens separated by one or more chars
   from sep string, not reentrant, temporarily modifies s
   typical use:
   for (t=tokenizer(b, " \t"); t; t=tokenizer(NULL, " \t"))
     { ... }
*/
extern char *tokenizer(char *s, char *sep);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   successively returns tokens separated by one or more chars
   from sep string, NOT reentrant
   typical use:
   for (t=ftokenizer(f, " \t"); t; t=tokenizer(NULL, " \t"))
     { ... }
*/
extern char *ftokenizer(FILE *f, char *sep);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   reverses a string s, returns a static buffer.
   NOT reentrant
*/
extern char *reverse(char *s);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   This function is a replacement for POSIX.1-2008 getdelim()
*/
size_t readdelim(char **lineptr, size_t  *n, int delim, FILE *stream);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   This function is a replacement for POSIX.1-2008 getline()
*/
size_t readline(char **lineptr, size_t  *n, FILE *stream);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   successively returns new lines from file, NOT reentrant
   typical use:
   for (t=freadline(f); t; t=freadline(f))
     { ... }
*/
extern char *freadline(FILE *f);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns substring of s with maximum length length
   - if length>0: starting at pos
     substr("12345", 1, 3)="234"
   - if length<0: ending with pos 
     substr("12345", 3, -2)="34"
*/
char *substr(char *s, int pos, int length);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns lowercase version of string, NOT reentrant,
   max. size==4096
*/
char *lowercase(char *);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns lowercase variant of c (including umlauts)
*/
char mytolower(char);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns length of common prefix
*/
int common_prefix_length(char *, char *);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   returns length of common suffix
*/
int common_suffix_length(char *, char *);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   frees the memory held by util.c.
*/
void util_teardown();

/* ------------------------------------------------------------ */
#endif
