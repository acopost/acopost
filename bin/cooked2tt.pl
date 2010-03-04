#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('h');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h]\n" if defined($opt_h) || $#ARGV!=-1;

$lno=0;
while ($l=<STDIN>) {
  $lno++;
  printf STDERR "%d\r", $lno if $lno%25==0;
  chomp($l);
  @ls=split(/\s+/, $l);
  for ($i=0; $i<$#ls; $i+=2) {
    printf "%s\t%s\n", $ls[$i], $ls[$i+1];
  }
  printf "\n";
}
printf STDERR "%d sentences\n", $lno;
