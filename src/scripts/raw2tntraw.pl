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

while ($l=<STDIN>) {
  chomp($l);
  @ws=split(/\s+/, $l);
  for ($i=0; $i<=$#ws; $i++) {
    printf "%s\n", $ws[$i];
  }
  printf "\n";
}
