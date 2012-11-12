BUILT_SOURCES = acopost-users-guide.ps acopost-users-guide.pdf

EXTRA_DIST = $(BUILT_SOURCES)

CLEANFILES = *~ $(BUILT_SOURCES) ug.ps ug.pdf \
             ug.dvi ug.log ug.bbl ug.toc ug.blg ug.aux

dist_doc_DATA = $(BUILT_SOURCES)


ug.dvi: ug.tex
	latex ug.tex
	bibtex ug
	latex ug.tex
	latex ug.tex

ug.ps: ug.dvi
	dvips -o ug.ps ug.dvi

ug.pdf: ug.ps
	ps2pdf14 ug.ps

acopost-users-guide.ps: ug.ps
	cp -fp $< $@

acopost-users-guide.pdf: ug.pdf
	cp -fp $< $@
