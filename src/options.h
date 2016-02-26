/*
  Some options utility functions
  
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdlib.h>
#include <stdio.h>

typedef struct option_callback_data_s {
  void* dataptr;
  int (*parser) (char* string, void *dataptr);
  int (*serializer) (void *dataptr, FILE *out);
} option_callback_data_t;
typedef option_callback_data_t *option_callback_data_pt;

typedef struct option_entry_s
{
  char character;
  char arg_type;
  void *dataptr;
  char *usage;
} option_entry_t;
typedef option_entry_t *option_entry_pt;

typedef struct option_context_s
{
  char *cmd;
  char *description;
  char *synopsis;
  char *banner;
  option_entry_pt ops;
} option_context_t;
typedef option_context_t *option_context_pt;

enum OPTION_ARG_TYPE {
	OPTION_NONE=0,
	OPTION_STRING=1,
	OPTION_SIGNED_LONG=2,
	OPTION_UNSIGNED_LONG=3,
	OPTION_DOUBLE=4,
	OPTION_CALLBACK=5
};

int options_parse(option_context_pt options, const char* terminator, int argc, char**argv);

int options_print_usage(option_context_pt options, FILE *out);

int options_print_configuration(option_context_pt options, FILE* out);

/* ------------------------------------------------------------ */
#endif
