#!/bin/bash
rm -f `find . -type f -a -name libtool`
rm -f `find . -type f -a -name gmon.out`
rm -f `find doc -type f -a -name *.log`
rm -f configure
rm -f ltmain.sh
rm -f libtool
rm -f install-sh
rm -f aclocal.m4
rm -f config.guess
rm -f config.sub
rm -f config.log
rm -f config.h
rm -f config.status
rm -f stamp-h* 
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
rm -rf src/.deps
rm -r tests/*.test
