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
	  printf("\t%s %d", (char *)array_get(l->tags, w->sorter[i]), w->tagcount[w->sorter[i]]);
	}
      printf("\n");
    }

}
