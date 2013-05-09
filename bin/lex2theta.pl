#!/usr/bin/env perl

#
# Ingo Schröder
#

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-h] [-r rwt] lexfile\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

$rwt = 5;
(
 'r=i' => \$rwt,
 'h|help'        => sub { usage (); exit },
);

die $Usage if $#ARGV!=0;

open(F, "<$ARGV[0]") || die "can't open \"$ARGV[0]\": $!\n";
LINE:
while ($l=<F>) {
  chomp($l);
  next LINE if $l=~m/^%%/;
  next LINE if $l=~m/^@/;
  if ($l=~m/^\S+\s+(\d+)\s+(.*)$/) {
    if ($1<=$rwt) {
      @as=split(/\s+/, $2);
      for (my $i=0; $i<=$#as; $i+=2) {
	$count{$as[$i]}+=$as[$i+1];
      }
    }
  } else {
    die "can't parse line \"$l\"\n";
  }
}
close(F);

foreach $t (keys %count) {
  printf "%d\n", $count{$t};
}
