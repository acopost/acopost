/*
  Options utility functions

  Copyright (c) 2016, ACOPOST Developers Team
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
*/

#include "options.h"
#include <string.h>

static const char* option_arg_type_to_string(enum OPTION_ARG_TYPE t) {
	switch(t){
	case OPTION_NONE:
		return "";
	case OPTION_STRING:
		return "<string>";
	case OPTION_SIGNED_LONG:
		return "<int>";
	case OPTION_UNSIGNED_LONG:
		return "<uint>";
	case OPTION_DOUBLE:
		return "<double>";
	case OPTION_CALLBACK:
		return "<value>";
	}
	return "null";
}


int options_print_configuration(option_context_pt options, FILE* out) {
	option_entry_pt opt = options->ops;
	option_callback_data_pt cdpt;
	while(opt->character) {
			switch(opt->arg_type){
			case OPTION_NONE:
				if(*((int*)opt->dataptr))
					fprintf(out, "-%c\n", opt->character);
				break;
			case OPTION_STRING:
				if(*((char**)opt->dataptr))
					fprintf(out, "-%c %s\n", opt->character, *((char**)opt->dataptr));
				break;
			case OPTION_SIGNED_LONG:
				fprintf(out, "-%c %ld\n", opt->character, *((long*)opt->dataptr));
				break;
			case OPTION_UNSIGNED_LONG:
				fprintf(out, "-%c %lu\n", opt->character, *((unsigned long*)opt->dataptr));
				break;
			case OPTION_DOUBLE:
				fprintf(out, "-%c %lf\n", opt->character, *((double*)opt->dataptr));
				break;
			case OPTION_CALLBACK:
				cdpt = (option_callback_data_pt) opt->dataptr;
				fprintf(out, "-%c", opt->character);
				if(cdpt->serializer) {
					fputc(' ', out);
					cdpt->serializer(cdpt->dataptr, out);
				}
				fputc('\n', out);
				break;
			}
		opt++;
	}
	fprintf(out, "\n");
	return 0;
}


int options_print_usage(option_context_pt options, FILE *out) {
	fprintf(out, "NAME\n\n  %s - %s\n\nSYNOPSYS\n\n  %s %s\n\n", options->cmd, options->description, options->cmd, options->synopsis);
	option_entry_pt opt = options->ops;
	if(opt->character) {
		fprintf(out, "OPTIONS\n\n");
	}
	while(opt->character) {
		fprintf(out, "  -%c %8s %s\n", opt->character, option_arg_type_to_string(opt->arg_type), opt->usage);
		opt++;
	}
	if(options->banner) {
		fprintf(out, "\nVERSION\n\n%s\n", options->banner);
	}
	return 0;
}

static inline int return_check(int argc, char**argv, const char* terminator, int i) {
	if(terminator == NULL)
		return i;
	if(i>=argc)
		return i;
	if(!strcmp(terminator, argv[i])) {
		return i+1;
	}
	return i;
}

int options_parse(option_context_pt options, const char* terminator, int argc, char**argv) {
	int i = 1;
	for(i=1;i<argc;i++) {
		if( (argv[i][0] == '-') && (argv[i][0] != '\0') ){
			option_entry_pt opt = options->ops;
			if( (argv[i][1] == '\0') || (argv[i][2] != '\0') ){
				/* fprintf(stderr, "Unknown option '%s'\n", argv[i]); */
				return return_check(argc, argv, terminator, i);
			}
			while(opt->character) {
				if(argv[i][1] == opt->character) {
					switch(opt->arg_type){
					case OPTION_NONE:
						*((int*)opt->dataptr) = 1;
						break;
					case OPTION_STRING:
						i++;
						if(i<argc) {
							*((char**)opt->dataptr) = argv[i];
						} else {
							fprintf(stderr, "Unable to parse %s for option '%s'\n", option_arg_type_to_string(opt->arg_type), argv[i-1]);
							return return_check(argc, argv, terminator, i-1);
						}
						break;
					case OPTION_SIGNED_LONG:
						i++;
						if(i<argc) {
							if (1!=sscanf(argv[i], "%ld", (long*)opt->dataptr))
							{
								fprintf(stderr, "Unable to parse %s for option '%s' with value \"%s\"\n", option_arg_type_to_string(opt->arg_type), argv[i-1], argv[i]);
								return return_check(argc, argv, terminator, i-1);
							}
						} else {
							fprintf(stderr, "Unable to parse %s for option '%s'\n", option_arg_type_to_string(opt->arg_type), argv[i-1]);
							return return_check(argc, argv, terminator, i-1);
						}
						break;
					case OPTION_UNSIGNED_LONG:
						i++;
						if(i<argc) {
							if (1!=sscanf(argv[i], "%lu", (unsigned long*)opt->dataptr))
							{
								fprintf(stderr, "Unable to parse %s for option '%s' with value \"%s\"\n", option_arg_type_to_string(opt->arg_type), argv[i-1], argv[i]);
								return return_check(argc, argv, terminator, i-1);
							}
						} else {
							fprintf(stderr, "Unable to parse %s for option '%s'\n", option_arg_type_to_string(opt->arg_type), argv[i-1]);
							return return_check(argc, argv, terminator, i-1);
						}
						break;
					case OPTION_DOUBLE:
						i++;
						if(i<argc) {
							if (1!=sscanf(argv[i], "%lf", (double*)opt->dataptr))
							{
								fprintf(stderr, "Unable to parse %s for option '%s' with value \"%s\"\n", option_arg_type_to_string(opt->arg_type), argv[i-1], argv[i]);
								return return_check(argc, argv, terminator, i-1);
							}
						} else {
							fprintf(stderr, "Unable to parse %s for option '%s'\n", option_arg_type_to_string(opt->arg_type), argv[i-1]);
							return return_check(argc, argv, terminator, i-1);
						}
						break;
					case OPTION_CALLBACK:
						i++;
						if(i<argc) {
							option_callback_data_pt cdpt = (option_callback_data_pt) opt->dataptr;
							if(cdpt->parser(argv[i], cdpt->dataptr)) {
								fprintf(stderr, "Unable to parse %s for option '%s' with value \"%s\"\n", option_arg_type_to_string(opt->arg_type), argv[i-1], argv[i]);
								return return_check(argc, argv, terminator, i-1);
							}
						} else {
							fprintf(stderr, "Unable to parse %s for option '%s'\n", option_arg_type_to_string(opt->arg_type), argv[i-1]);
							return return_check(argc, argv, terminator, i-1);
						}
						break;
					}
					break;
				}
				opt++;
			}
			if(!opt->character) {
				return return_check(argc, argv, terminator, i);
			}
		} else {
			return return_check(argc, argv, terminator, i);
		}
	}
	return return_check(argc, argv, terminator, i);
}
