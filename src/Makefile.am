SUBDIRS = scripts

if DEBUG
AM_CFLAGS = -O0 -g3 -DDEVELOPMENT_CHECKS
else
AM_CFLAGS = -O2
endif

# default is double
AM_CFLAGS += -Wall -Wsign-compare -pedantic -std=c99 -D_USE_BSD -DT3_PROB_IS_FLOAT


bin_PROGRAMS = acopost-et acopost-met acopost-t3 acopost-tbt
noinst_PROGRAMS = lextest acopost_test eqsort_test util_test

noinst_HEADERS = array.h config-common.h gis.h hash.h lexicon.h mem.h primes.h util.h sregister.h iregister.h eqsort.h
LIBRARY_FILES = array.c mem.c util.c hash.c primes.c sregister.c iregister.c eqsort.c

acopost_et_SOURCES = et.c $(LIBRARY_FILES)
acopost_et_LDFLAGS = -lm

acopost_met_SOURCES = met.c gis.c $(LIBRARY_FILES)
acopost_met_LDFLAGS = -lm

acopost_t3_SOURCES = t3.c $(LIBRARY_FILES)
acopost_t3_LDFLAGS = -lm

acopost_tbt_SOURCES = tbt.c $(LIBRARY_FILES)
acopost_tbt_LDFLAGS = -lm

lextest_SOURCES = lexicon.c lextest.c $(LIBRARY_FILES)
lextest_LDFLAGS = -lm

acopost_test_SOURCES = acopost_test.c $(LIBRARY_FILES)
acopost_test_LDFLAGS = -lm

eqsort_test_SOURCES = eqsort_test.c $(LIBRARY_FILES)
eqsort_test_LDFLAGS = -lm

util_test_SOURCES = util_test.c $(LIBRARY_FILES)
util_test_LDFLAGS = -lm



CLEANFILES = *.o $(bin_PROGRAMS) $(noinst_PROGRAMS) *~ core
