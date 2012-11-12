t3dir = $(pkgdatadir)/language-models/t3

BUILT_SOURCES = danish-pre-1948.lex2  danish-pre-1948.ngram2

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = $(BUILT_SOURCES) \
             danish-pre-1948/lex2 danish-pre-1948/ngram2

dist_t3_DATA = $(BUILT_SOURCES)


danish-pre-1948.lex2: danish-pre-1948/lex2
	cp -fp $< $@

danish-pre-1948.ngram2: danish-pre-1948/ngram2
	cp -fp $< $@

