/*
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

#include "config.h"
#include <stdio.h>
#include <string.h>

#include "lexicon.h"
#include "hash.h"
#include "array.h"
#include "util.h"
#include "mem.h"

int main(int argc, char **argv)
{
  lexicon_pt l=read_lexicon_file(argv[1]);
  char s[4000];
  
  printf("Read %ld lexical entries.\n", (long int)hash_size(l->words));
  while (fgets(s, 1000, stdin))
    {
      word_pt w;
      int i, sl=strlen(s);

      for (sl--; sl>=0 && s[sl]=='\n'; sl--) { s[sl]='\0'; }
      w=hash_get(l->words, s);
      if (!w) { printf("No such word \"%s\".\n", s); continue; }

      printf("%s[%d,%s]:", s, w->defaulttag, tagname(l, w->defaulttag));
      for (i=0; w->sorter[i]>0; i++)
	{
	  printf("\t%s %d", tagname(l, w->sorter[i]), w->tagcount[w->sorter[i]]);
	}
      printf("\n");
    }

  return 0;
}
