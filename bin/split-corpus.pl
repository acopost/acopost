#!/usr/bin/env perl

#
# Ingo Schröder
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

use FileHandle;

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-h] [-m modulo] [-p prefix] [-v] [corpusfile]\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

$p = "";
$m = 10;
$opt_v = 0;
GetOptions
(
 'p=s' => \$p,
 'm=i' => \$m,
 'v' => \$opt_v,
 'h|help'        => sub { usage (); exit },
);

die $Usage if  $#ARGV>0;

if ($#ARGV==0) {
  open(STDIN, "<$ARGV[0]") || die "$cmd: can't open corpus file \"$ARGV[0]\": $!\n";
  $ARGV[0]=~s/.cooked$//;
  if($p eq "")
  {
    $p = $ARGV[0];
  }
}
if($p eq "")
{
  $p = "pre";
}

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
