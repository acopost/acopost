#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('h');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h]\n" if defined($opt_h);

$lno=0;
while ($l=<STDIN>) {
  chomp($l);
  $lno++;
  next if $l=~m/^%%/;
  if ($l=~m/([^\s]+)\s+([^\s]+)/) {
    push(@s, $1);
    push(@t, $2);
  } else {
    next if $#s<0;
    for ($i=0; $i<=$#s; $i++) {
      printf " " if $i!=0;
      printf "%s %s", $s[$i], $t[$i];
    }
    printf "\n";
    @s=(); @t=();
    printf STDERR "%d\r", $lno if $lno%25==0;
  }
}
printf STDERR "%d lines read\n", $lno;
