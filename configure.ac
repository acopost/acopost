#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([acopost], [2.0.0.pre01], [ulrikp@users.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/t3.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
# AC_PROG_LEX

# Checks for libraries.
AC_CHECK_LIB([m], [log])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h values.h string.h math.h locale.h sys/resource.h])
# Checks for functions.
AC_CHECK_FUNCS(nice srand48 drand48 strdup)


# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor memmove memset sqrt strchr strcspn strdup strerror strpbrk strrchr strspn strstr])

# Check for presence of LaTeX tools
AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
AM_CONDITIONAL([HAVE_PDFLATEX], test -n "$PDFLATEX")
AC_CHECK_PROG(LATEX, latex, latex)
AM_CONDITIONAL([HAVE_LATEX], test -n "$LATEX")
AC_CHECK_PROG(BIBTEX, bibtex, bibtex)
AM_CONDITIONAL([HAVE_BIBTEX], test -n "$BIBTEX")
AC_CHECK_PROG(DVIPS, dvips, dvips)
AM_CONDITIONAL([HAVE_DVIPS], test -n "$DVIPS")
AC_CHECK_PROG(PDF2PS, pdf2ps, pdf2ps)
AM_CONDITIONAL([HAVE_PDF2PS], test -n "$PDF2PS")
AC_CHECK_PROG(PS2PDF, ps2pdf, ps2pdf)
AM_CONDITIONAL([HAVE_PS2PDF], test -n "$PS2PDF")



AC_CONFIG_FILES([doc/Makefile
                 doc/ug/Makefile
                 language-models/Makefile
                 language-models/t3/Makefile
                 bin/Makefile
                 src/Makefile
                 Makefile
                 acopost.spec])
AC_OUTPUT
