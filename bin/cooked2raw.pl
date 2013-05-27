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
  $lno++;
  printf STDERR "%d\r", $lno;
  chomp($l);
  @ls=split(/\s+/, $l);
  for ($i=0; $i<$#ls; $i+=2) {
    printf " " if $i>0;
    printf "%s", $ls[$i];
  }
  printf "\n";
}
