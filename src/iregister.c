#include "config-common.h"
#include "iregister.h"
#include "array.h"
#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdarg.h>
#include <errno.h>



struct iregister_s {
	hash_pt g_table;/* lookup table tags[taghash{"tag"}-1]="tag */
	array_pt g_array;/* tags */
	size_t cp;
};

/* ------------------------------------------------------------ */
const char *iregister_get_name(iregister_pt l, ptrdiff_t i)
{
	return (char *)array_get(l->g_array, i);
}

/* ------------------------------------------------------------ */
ptrdiff_t iregister_get_index(iregister_pt l, const char *t)
{
	return (ptrdiff_t)hash_get(l->g_table, (char*)t)-1;
}

/* ------------------------------------------------------------ */
ptrdiff_t iregister_add_name(iregister_pt l, const char *t)
{
	ptrdiff_t i=iregister_get_index(l, t);

	if (i<0) 
	{ 
		char *rt=strdup(t);
		i=array_add(l->g_array, rt);
		hash_put(l->g_table, (char*)rt, (void *)(i+1));
	}
	return i;
}

size_t iregister_get_length(iregister_pt l) {
	return array_count(l->g_array);
}



iregister_pt iregister_new(size_t cp) {
	iregister_pt l = (iregister_pt)malloc(sizeof(iregister_s));
	if(l == NULL)
		return NULL;
	l->g_array=array_new(cp/2);
	l->g_table=hash_new(cp, .6, hash_string_hash, hash_string_equal);
	l->cp = cp;
	return l;
}

size_t iregister_add_unregistered_name(iregister_pt l, const char* s) {
	return array_add(l->g_array, strdup(s));
}

void iregister_delete(iregister_pt l) {
	iregister_clear(l);
	hash_delete(l->g_table);
	array_free(l->g_array);
}

void iregister_clear(iregister_pt l) {
	size_t i;
	hash_clear(l->g_table);

	/* Clear the g_array array. */
	size_t not=array_count(l->g_array);
	for (i = 0; i < not; ++i) {
		/* Free the string in m->g_array.
		 * Do NOT use mem_free, since the memory has been obtained with strdup.
		 */
		free(array_get(l->g_array, i));
	}

	array_clear(l->g_array);
}

