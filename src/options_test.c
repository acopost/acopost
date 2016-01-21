#include "options.h"
#include "option_mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char*argv[]) {

	int h = 0;
	unsigned long v = 1;
	long i = 0;
	char *l = NULL;
	long m = 1;
	long n = 0;
	long o = 0;
	char *p = NULL;
	int r = 0;
	char *t = NULL;
	char *u = NULL;
	double d = 0.0;
	int C = 0;
	option_callback_data_t cd = {
		&C,
		option_operation_mode_parser,
		option_operation_mode_serializer
	};
	option_context_t options = {
		"Transformation-based Tagger (c) Ingo Schröder and others, http://acopost.sf.net/",
		argv[0],
		" OPTIONS rulefile [inputfile]",
		(option_entry_t[]) {
			{ 'h', OPTION_NONE, (void*)&h, "display this help" },
			{ 'v', OPTION_UNSIGNED_LONG, (void*)&v, "verbosity level [1]" },
			{ 'i', OPTION_SIGNED_LONG, (void*)&i, "maximum number of iterations [unlimited]" },
			{ 'l', OPTION_STRING, (void*)&l, "lexicon file [none]" },
			{ 'm', OPTION_SIGNED_LONG, (void*)&m, "minimum improvement per iteration [1]" },
			{ 'n', OPTION_SIGNED_LONG, (void*)&n, "rare wore threshold [0]" },
			{ 'o', OPTION_SIGNED_LONG, (void*)&o, "mode of operation 0 tagging, 1 testing, 2 training [0]" },
			{ 'p', OPTION_STRING, (void*)&p, "preload file [none]" },
			{ 'r', OPTION_NONE, (void*)&r, "assume raw format for input [cooked format]" },
			{ 't', OPTION_STRING, (void*)&t, "template file [none]" },
			{ 'u', OPTION_STRING, (void*)&u, "unknown word default tag [lexicon based]" },
			{ 'd', OPTION_DOUBLE, (void*)&d, "test double [0.0]" },
			{ 'C', OPTION_CALLBACK, (void*)&cd, "test double [0.0]" },
			{ '\0', OPTION_NONE, NULL, NULL }
		}
	};
	int idx = options_parse(&options, "--", argc, argv);
	if(h) {
		options_print_usage(&options, stdout);
		return 0;
	}
	if(idx < argc) {
		fprintf(stderr, "First unrecognized parameter: %s\n", argv[idx]);
	}
			
	options_print_configuration(&options, stdout);
	return 0;
}
