#!/usr/bin/env perl

#
# Ingo Schröder
#

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-h]\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

GetOptions
(
 'h|help'        => sub { usage (); exit },
);

die $Usage if $#ARGV!=-1;

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
