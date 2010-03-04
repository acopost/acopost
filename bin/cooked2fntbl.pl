#!/usr/bin/env perl

#
# Ingo Schröder
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

use FileHandle;

require "getopts.pl";

&Getopts('hv');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h] [-v] lexicon\n"
  if defined($opt_h) || defined($opt_h) || $#ARGV!=0;

$nle=0;
open(F, "<$ARGV[0]") || die "can't open lexicon \"$ARGV[0]\": $!\n";
while ($l=<F>) {
  if ($l=~m/^(\S+)\s+(.+)$/) {
    $nle++;
    printf STDERR "%d lexical entries read\r", $nle if defined($opt_v) && $nle%41==0;
    my $w=$1;
    my @vs=split(/\s+/, $2);
    $lexicon{$w}=$vs[0];
    for (my $i=0; $i<=$#vs; $i+=2) {
      $tc{$vs[$i]}+=$vs[$i+1];
    }
  }
}
close(F);
printf STDERR "%d lexical entries read\n", $nle if defined($opt_v);
@tags=sort { $tc{$b} <=> $tc{$a} } keys %tc;
$mft=$tags[0];
printf STDERR "most frequent tag %s\n", $mft if defined($opt_v);

$nos=0;
LINE:
while ($l=<STDIN>) {
  next LINE if $l=~m/^\s*$/;
  $nos++;
  printf STDERR "%d sentences read\r", $nos if defined($opt_v) && $nos%41==0;
  chomp($l);
  my @as=split(/\s+/, $l);
  for (my $i=0; $i<=$#as; $i+=2) {
    my $w=$as[$i];
    my $t=$as[$i+1];
    printf "%s %s %s\n", $w, defined($lexicon{$w}) ? $lexicon{$w} : $mft, $t;
  }
  printf "\n";
}
printf STDERR "%d sentences read\n", $nos if defined($opt_v);
