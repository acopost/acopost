

all: ug.ps ug.pdf

ug.ps: ug.dvi Makefile
	dvips -o ug.ps ug.dvi

ug.dvi: ug.tex Makefile
	latex ug.tex
	bibtex ug
	latex ug.tex
	latex ug.tex

clean::
	rm -f *~ ug.ps ug.pdf ug.dvi ug.log ug.bbl ug.toc ug.blg ug.aux

ug.pdf: ug.ps
	ps2pdf14 ug.ps
