/*
  Some utility functions
  
  Copyright (C) 2001 Ingo Schröder
  Copyright (C) 2013 Ulrik Sandborg-Petersen

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
   compares to ints, for qsort()
*/
extern int intcompare(const void *ip, const void *jp);

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
   successively returns new lines from file, NOT reentrant
   typical use:
   for (t=freadline(f); t; t=freadline(f))
     { ... }
*/
extern char *freadline(FILE *f);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   looks for s in string table
   - if found, returns stored string
   - if not, copies s, stores the copy and returns the copy
*/
char *register_string(char *s);

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
