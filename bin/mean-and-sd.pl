#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('hcs');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-c] [-h]\n" if defined($opt_h) || $#ARGV!=-1;

$c=0;
while ($l=<STDIN>) {
  chomp($l);
  push(@a, $l);
  $c+=$l;
}

$sum=0.0;
for ($i=0; $i<=$#a; $i++) {
  $a[$i]/=$c if defined($opt_c);
  $sum+=$a[$i];
}
$mean=$sum/($#a+1);

$sum=0.0;
for ($i=0; $i<=$#a; $i++) {
  my $x=$a[$i]-$mean;
  $sum+=($x*$x);
}

$var=$sum/$#a;
$sd=sqrt($var);

if (defined($opt_s)) {
  printf "mean %f var %f sd %f\n", $mean, $var, $sd;
} else {
  printf "count: %d\n", $c if defined($opt_c);
  printf " mean: %f\n", $mean;
  printf "  var: %f\n", $var;
  printf "   sd: %f\n", $sd;
}
