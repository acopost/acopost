

all: ug.ps

ug.ps: ug.dvi Makefile
	dvips -o ug.ps ug.dvi

ug.dvi: ug.tex Makefile
	latex ug.tex
	bibtex ug
	latex ug.tex
	latex ug.tex

clean::
	rm -f *~ ug.ps ug.dvi ug.log ug.bbl ug.toc ug.blg ug.aux
