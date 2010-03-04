#!/usr/bin/env perl

#
# Ingo Schröder
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

use FileHandle;

require "getopts.pl";

&Getopts('hm:p:v');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h] [-m modulo] [-p prefix] [-v] [corpusfile]\n"
  if defined($opt_h) || defined($opt_h) || $#ARGV>0;

if ($#ARGV==0) {
  open(STDIN, "<$ARGV[0]") || die "$cmd: can't open corpus file \"$ARGV[0]\": $!\n";
  $ARGV[0]=~s/.cooked$//;
}
$m=defined($opt_m) ? $opt_m : 10;
$p=defined($opt_p) ? $opt_p : $#ARGV==0 ? $ARGV[0] : "pre";

for ($i=0; $i<$m; $i++) {
  my $fh=new FileHandle;
  $fh->open(">$p-$i") || die "can't open file \"$p-$i\": $!\n";
  push(@fhs, $fh);
}

for ($i=0; $l=<STDIN>; $i++) {
  my $fh=$fhs[$i%$m];
  printf "%d\r", $i;
  print $fh $l;
}

foreach $fh (@fhs) {
  $fh->close;
}
