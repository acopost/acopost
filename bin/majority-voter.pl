#!/usr/bin/env perl

#
# Ingo Schröder, ingo.schroeder@nats.informatik.uni-hamburg.de
#

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-h] reference a b ...\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

$opt_q = 0;
GetOptions
(
 'q'             => \$opt_q,
 'h|help'        => sub { usage (); exit },
);

die $Usage if $#ARGV<2;

$lno=0;
open(R, "<$ARGV[0]") || die "can't open \"reference\" file: $!\n";
for ($i=1; $i<=$#ARGV; $i++) {
  local *FH;
  open(FH, "<$ARGV[$i]") || die "can't open \"$ARGV[$i]\" file: $!\n";
  $fh[$i]=*FH;
}

$wc=0;
while (!eof(R)) {
  $r=<R>;
  # printf STDERR "R: %s", $r;
  chomp($r);
  # printf STDERR "%d\n", $#ARGV;
  for ($i=1; $i<=$#ARGV; $i++) {
    my $fh=$fh[$i];
    # printf STDERR "i=%d\n", $i;
    $l[$i]=<$fh>;
    # printf STDERR "%d: %s\n", $i, $l[$i];
    chomp($l[$i]);
  }
  $lno++;
  printf STDERR "%12d sentences\r", $lno unless $opt_q;
  @rs=split(/\s+/, $r);
  for ($i=0; $i<=$#rs; $i+=2) {
    my $nc=0;
    my $rw=$rs[$i];
    my $rt=$rs[$i+1];
    $wc++;
    %tseen=();
    for ($j=1; $j<=$#ARGV; $j++) {
      @ts=split(/\s+/, $l[$j]);
      my $tw=$ts[$i];
      my $tt=$ts[$i+1];
      if ($rw ne $tw) { die "ERROR: different words \"$rw\" \"$tw\" ($ARGV[$j]:$lno:$i)\n" }
      $nc++ if $rt eq $tt;
      $tseen{$tt}++;
    }
    foreach my $k (keys %tseen) {
      $tc[$tseen{$k}]++;
    }
    $count[$nc]++;
  }
}
printf "%d sentences                         \n", $lno;
close(R);
for ($i=1; $i<=$#ARGV; $i++) { close($fh[$i]); }

printf "%d words\n", $wc;
$tsum=$sum=0;
for ($i=$#ARGV; $i>=0; $i--) {
  $sum+=$count[$i];
  $tsum+=$tc[$i];
  printf "%d: %7.3f%% %7.3f%%", $i, 100*$count[$i]/$wc, 100*$sum/$wc;
  printf " %7.6f", $tsum/$wc if $i>0;
  printf "\n";
}
