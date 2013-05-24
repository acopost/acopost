AM_CFLAGS = -Wall -std=c99 -O2 -D_USE_BSD

# default is double
AM_CFLAGS += -DT3_PROB_IS_FLOAT


bin_PROGRAMS = et met t3 tbt
noinst_PROGRAMS = ltest

et_SOURCES = et.c array.c mem.c util.c hash.c primes.c
et_LDFLAGS = -lm

met_SOURCES = met.c gis.c array.c mem.c util.c hash.c primes.c 
met_LDFLAGS = -lm

t3_SOURCES = t3.c util.c mem.c array.c hash.c primes.c 
t3_LDFLAGS = -lm

tbt_SOURCES = tbt.c util.c mem.c array.c hash.c primes.c 
tbt_LDFLAGS = -lm

ltest_SOURCES = lexicon.c ltest.c array.c mem.c util.c hash.c primes.c
ltest_LDFLAGS = -lm


noinst_HEADERS = array.h config-common.h gis.h hash.h lexicon.h mem.h primes.h util.h


CLEANFILES = *.o $(bin_PROGRAMS) $(noinst_PROGRAMS) *~ core
