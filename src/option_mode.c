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

#include "option_mode.h"
#include <stdio.h>
#include <string.h>

int option_operation_mode_parser(char* string, void* data) {
	if(!strcmp("0", string)) {
		*((int*) data) = 0;
	}
	else if(!strcmp("1", string)) {
		*((int*) data) = 1;
	}
	else if(!strcmp("2", string)) {
		*((int*) data) = 2;
	}
	else if(!strcmp("7", string)) {
		*((int*) data) = 7;
	}
	else if(!strcmp("8", string)) {
		*((int*) data) = 8;
	} else if(!strcmp("tag", string)) {
		*((int*) data) = 0;
	}
	else if(!strcmp("test", string)) {
		*((int*) data) = 1;
	}
	else if(!strcmp("train", string)) {
		*((int*) data) = 2;
	}
	else if(!strcmp("dump", string)) {
		*((int*) data) = 7;
	}
	else if(!strcmp("debug", string)) {
		*((int*) data) = 8;
	} else {
		return 1;
	}
	return 0;
}

int option_operation_mode_serializer(void* data, FILE* out) {
	switch(*((int*)data)) {
	case 0:
		fprintf(out, "%s", "tag");
		break;
	case 1:
		fprintf(out, "%s", "test");
		break;
	case 2:
		fprintf(out, "%s", "train");
		break;
	case 7:
		fprintf(out, "%s", "dump");
		break;
	case 8:
		fprintf(out, "%s", "debug");
		break;
	}
	return 0;
}
