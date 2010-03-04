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
  printf STDERR "%d\r", $lno;
  chomp($l);
  @ls=split(/\s+/, $l);
  for ($i=0; $i<$#ls; $i+=2) {
    $uni{$ls[$i+1]}++;
    $bi{$ls[$i-1]}{$ls[$i+1]}++ if $i>0;
    $tri{$ls[$i-3]}{$ls[$i-1]}{$ls[$i+1]}++ if $i>2;
  }
}

foreach $i (keys %uni) {
  printf "%s %d\n", $i, $uni{$i};
  foreach $j (keys %uni) {
    printf "\t%s %d\n", $j, $bi{$i}{$j} if $bi{$i}{$j}>0;
    foreach $k (keys %uni) {
      printf "\t\t%s %d\n", $k, $tri{$i}{$j}{$k} if $tri{$i}{$j}{$k}>0;
    }
  }
}
