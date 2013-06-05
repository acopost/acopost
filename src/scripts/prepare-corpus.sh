#!/bin/bash

function die () {
    echo $1
    exit
}

corpus=$1

[ -z "$corpus" ] && die "give the corpus in cooked format as the first (and only) argument"

stem=`echo $corpus | sed -e 's/.cooked//'`

cooked2lex < $corpus > $stem.lex
split-corpus -p $stem -m 10 < $corpus 
for i in $stem-?; do
  mv $i $i.cooked
done

for i in 0 1 2 3 4 5 6 7 8 9; do
  for j in 0 1 2 3 4 5 6 7 8 9; do
    if [ $i -ne $j ]; then
      cat $stem-$j.cooked
    fi
  done > $stem-train-$i.cooked
done
