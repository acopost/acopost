#!/bin/sh

test -f Makefile && make distclean
rm -f aclocal.m4
rm -rf autom4te.cache/
rm -f `find . -name Makefile.in`
rm -f configure
rm -f depcomp
rm -f install-sh
rm -f missing

rm -f `find . -type f -a -name libtool`
rm -f `find . -type f -a -name gmon.out`
rm -f `find doc -type f -a -name *.log`
rm -f ltmain.sh
rm -f libtool
rm -f config.guess
rm -f config.sub
rm -f compile
rm -f mkinstalldirs

rm -f configure.scan
rm -f valgrind.out.txt out.txt
