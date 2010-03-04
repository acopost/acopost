#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('h');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h]\n" if defined($opt_h);

while ($l=<STDIN>) {
  chomp($l);
  @ws=split(/\s+/, $l);
  for ($i=0; $i<=$#ws; $i++) {
    printf "%s\n", $ws[$i];
  }
  printf "\n";
}
