LATEX_MAIN_FILES = acopost-users-guide.tex
BIBTEX_FILES = acopost-users-guide.bib
EXTRA_DIST = $(LATEX_MAIN_FILES) $(BIBTEX_FILES)


CLEANFILES = *~ $(BUILT_SOURCES)

if HAVE_BIBTEX
CLEANFILES += $(BIBTEX_FILES:.bib=.blg) $(BIBTEX_FILES:.bib=.bbl)
endif

if HAVE_LATEX
CLEANFILES += $(LATEX_MAIN_FILES:.tex=.ps) $(LATEX_MAIN_FILES:.tex=.pdf)
EXTRA_DIST += $(LATEX_MAIN_FILES:.tex=.pdf)
endif
CLEANFILES += acopost-users-guide.ps acopost-users-guide.pdf \
             acopost-users-guide.dvi

dist_doc_DATA = $(BUILT_SOURCES)
doc_DATA=

if HAVE_BIBTEX

if HAVE_PDFLATEX
CLEANFILES += $(LATEX_MAIN_FILES:.tex=.pdf) $(LATEX_MAIN_FILES:.tex=.aux) $(LATEX_MAIN_FILES:.tex=.toc) $(LATEX_MAIN_FILES:.tex=.out) $(LATEX_MAIN_FILES:.tex=.log)

.tex.blg:
	$(PDFLATEX) $<
	BIBINPUTS=$(srcdir) $(BIBTEX) $*.aux
	$(RM) $*.pdf

.tex.pdf:
.blg.pdf:
	mkdir -p .pdflatex
	TEXMFOUTPUT=.pdflatex $(PDFLATEX) --output-directory .pdflatex $(srcdir)/$*.tex
	TEXMFOUTPUT=.pdflatex $(PDFLATEX) --output-directory .pdflatex $(srcdir)/$*.tex
	mv .pdflatex/$@ $@

clean-local::
	-rm -rf .pdflatex

doc_DATA += $(LATEX_MAIN_FILES:.tex=.pdf)
endif

if HAVE_LATEX

if !HAVE_PDFLATEX
.tex.blg:
	$(LATEX) $<
	BIBINPUTS=$(srcdir) $(BIBTEX) $*.aux
	$(RM) $*.dvi
endif

if HAVE_DVIPS
CLEANFILES += $(LATEX_MAIN_FILES:.tex=.ps) $(LATEX_MAIN_FILES:.tex=.dvi) $(LATEX_MAIN_FILES:.tex=.aux) $(LATEX_MAIN_FILES:.tex=.toc) $(LATEX_MAIN_FILES:.tex=.out) $(LATEX_MAIN_FILES:.tex=.log)

.blg.dvi:
.tex.dvi:
	mkdir -p .latex
	TEXMFOUTPUT=.latex $(LATEX) --output-directory .latex $(srcdir)/$*.tex
	TEXMFOUTPUT=.latex $(LATEX) --output-directory .latex $(srcdir)/$*.tex

.dvi.ps:
	$(DVIPS) .latex/$<

clean-local::
	-rm -rf .latex

doc_DATA += $(LATEX_MAIN_FILES:.tex=.ps)

if !HAVE_PDFLATEX
if HAVE_PS2PDF
CLEANFILES += $(LATEX_MAIN_FILES:.tex=.pdf)

.ps.pdf:
	$(PS2PDF) $<

doc_DATA += $(LATEX_MAIN_FILES:.tex=.pdf)
endif
endif

endif

else

if HAVE_PDFLATEX
if HAVE_PDF2PS
CLEANFILES += $(LATEX_MAIN_FILES:.tex=.ps)

.pdf.ps:
	$(PDF2PS) $<

doc_DATA += $(LATEX_MAIN_FILES:.tex=.ps)
endif
endif

endif

endif
