#!/bin/bash

function Die () {
    echo $1
    exit 1
}

[ -d "$1" ] || Die "Can't find directory \"$1\""

d=$1

mkdir $d/src
mkdir $d/bin
mkdir $d/html
mkdir $d/html/met
mkdir $d/html/t3
mkdir $d/html/tbt

cp /home/ingo/dawai/cdg/LICENSE $d
cp README $d

for i in \
    icopost/Makefile \
    icopost/array.[ch] \
    icopost/hash.[ch] \
    icopost/mem.[ch] \
    icopost/primes.[ch] \
    icopost/util.[ch] \
    icopost/met.c \
    icopost/gis.[ch] \
    icopost/t3.c \
    icopost/tbt.c \
    ; do
    cp $i $d/src/
done

wd="/home/ingo/public_html/icopost"

for i in \
    index.html \
    met/index.html \
    met/me.gif \
    met/tsize.gif \
    t3/index.html \
    t3/theta.gif \
    tbt/index.html \
    tbt/rules.gif \
    ; do
    cp $wd/$i $d/html/$i
done

for i in \
    bin/cooked2lex.pl \
    bin/cooked2ngram.pl \
    bin/cooked2raw.pl \
    bin/cooked2tt.pl \
    bin/tt2cooked.pl \
    bin/evaluate-tagged-against-reference.pl \
    ; do
    cp $i $d/bin
done
