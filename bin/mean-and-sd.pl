#!/usr/bin/env perl

#
# Ingo Schröder
#

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-c] [-h]\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

$opt_c = 0;
$opt_s = 0;
GetOptions
(
 'c'             => \$opt_c,
 's'             => \$opt_s,
 'h|help'        => sub { usage (); exit },
);

die $Usage if $#ARGV!=-1;

$c=0;
while ($l=<STDIN>) {
  chomp($l);
  push(@a, $l);
  $c+=$l;
}

$sum=0.0;
for ($i=0; $i<=$#a; $i++) {
  $a[$i]/=$c if $opt_c;
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

if ($opt_s) {
  printf "mean %f var %f sd %f\n", $mean, $var, $sd;
} else {
  printf "count: %d\n", $c if $opt_c;
  printf " mean: %f\n", $mean;
  printf "  var: %f\n", $var;
  printf "   sd: %f\n", $sd;
}
