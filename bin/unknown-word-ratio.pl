#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('h');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h]\n" if defined($opt_h) || $#ARGV!=-1;

$step=10000;
$oldwc=0;
$unknown=0;
$lno=0;
LINE:
while ($l=<STDIN>) {
  $lno++;
  next LINE if $l=~m/^\s*$/;
  printf STDERR "%d\r", $lno if $lno%1000==0;
  chomp($l);
  @ws=split(/\s+/, $l);
 WORD:
  for ($i=0; $i<=$#ws; $i++) {
    my $w=$ws[$i];
    next WORD unless $w=~m/^[A-ZÄÖÜa-zäöüß\-]+$/;
    #printf "%s ", $w;
    $wc++;
    #$w=~s/[.?!;,:]+$//;
    $unknown++ unless defined($c{$w});
    $c{$w}++;
    if ($wc==$oldwc+$step) {
      my @words=keys %c;
      printf "%15d %15d %7.3f%% %15d\n", $wc, $unknown, 100*$unknown/$step, $#words+1;
      $unknown=0;
      $oldwc=$wc;
    }
  }
}

