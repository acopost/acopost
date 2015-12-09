SUBDIRS = scripts

if DEBUG
AM_CFLAGS = -O0 -g3 -DDEVELOPMENT_CHECKS
else
AM_CFLAGS = -O2
endif

# default is double
AM_CFLAGS += -Wall -std=c99 -D_USE_BSD -DT3_PROB_IS_FLOAT


bin_PROGRAMS = acopost-et acopost-met acopost-t3 acopost-tbt
noinst_PROGRAMS = lextest acopost_test

acopost_et_SOURCES = et.c array.c mem.c util.c hash.c primes.c sregister.c
acopost_et_LDFLAGS = -lm

acopost_met_SOURCES = met.c gis.c array.c mem.c util.c hash.c primes.c sregister.c
acopost_met_LDFLAGS = -lm

acopost_t3_SOURCES = t3.c util.c mem.c array.c hash.c primes.c sregister.c
acopost_t3_LDFLAGS = -lm

acopost_tbt_SOURCES = tbt.c util.c mem.c array.c hash.c primes.c sregister.c
acopost_tbt_LDFLAGS = -lm

lextest_SOURCES = lexicon.c lextest.c array.c mem.c util.c hash.c primes.c sregister.c
lextest_LDFLAGS = -lm

acopost_test_SOURCES = acopost_test.c array.c mem.c util.c hash.c primes.c sregister.c
acopost_test_LDFLAGS = -lm

noinst_HEADERS = array.h config-common.h gis.h hash.h lexicon.h mem.h primes.h util.h sregister.h


CLEANFILES = *.o $(bin_PROGRAMS) $(noinst_PROGRAMS) *~ core
