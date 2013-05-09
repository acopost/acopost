BUILT_SOURCES = acopost-users-guide.ps acopost-users-guide.pdf

EXTRA_DIST = $(BUILT_SOURCES) ug.tex ug.bib

CLEANFILES = *~ $(BUILT_SOURCES) ug.ps ug.pdf \
             ug.dvi ug.log ug.bbl ug.toc ug.blg ug.aux

dist_doc_DATA = $(BUILT_SOURCES)


ug.pdf: ug.tex
	pdflatex ug.tex
	bibtex ug
	pdflatex ug.tex
	pdflatex ug.tex

ug.ps: ug.pdf ug.tex
	pdf2ps ug.pdf

acopost-users-guide.ps: ug.ps
	cp -fp $< $@

acopost-users-guide.pdf: ug.pdf
	cp -fp $< $@
