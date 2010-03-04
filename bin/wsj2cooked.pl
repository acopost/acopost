#!/usr/bin/env perl

#
# Ingo Schröder, schroeder@informatik.uni-hamburg.de
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

$tnow=$lno=0;
while ($l=<STDIN>) {
  $lno++;
  chomp($l);
  @ps=split(/\s+/, $l);
  $now=0;
  for (my $i=0; $i<=$#ps; $i++) {
    my $p=$ps[$i];
    if ($p=~m/^(\S+)\/(\S+)$/) {
      printf "%s%s %s", $i==0 ? "" : " ", $1, $2;
      $now++;
    } else {
      printf STDERR "line %d: weird word/tag pair: %s\n", $p;
    }
  }
  printf "\n" if $now>0;
  $tnow+=$now;
}
printf STDERR "no. of sentences: %d\n", $lno;
printf STDERR "    no. of words: %d\n", $tnow;
