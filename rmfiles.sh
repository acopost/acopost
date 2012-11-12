#!/bin/bash
rm -f `find . -type f -a -name libtool`
rm -f `find . -type f -a -name gmon.out`
rm -f configure
rm -f ltmain.sh
rm -f libtool
rm -f install-sh
rm -f aclocal.m4
rm -f config.guess
rm -f config.sub
rm -f config.log
rm -f configure.scan
rm -f depcomp
rm -f missing
rm -f mkinstalldirs
rm -f compile
rm -f `find . -name Makefile.in`
rm -f `find . -name Makefile`
rm -rf autom4te.cache/
rm -f acopost.spec
rm -f *~
