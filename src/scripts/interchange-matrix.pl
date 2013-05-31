#!/usr/bin/env perl

#
# Ingo Schröder
#

$cmd=$0;
$cmd=~s/(.*\/)*//;
$Usage="Usage: $cmd [-h] tagged reference\n";

use Getopt::Long;
Getopt::Long::Configure(qw( auto_abbrev no_ignore_case ));

sub usage
{
    print $Usage;
}

$opt_q = 0;
$opt_v = 0;
GetOptions
(
 'q' => \$opt_q,
 'v' => \$opt_v,
 'h|help'        => sub { usage (); exit },
);

die $Usage if $#ARGV!=1;

$pos=$neg=$lno=0;
open(T, "<$ARGV[0]") || die "can't open \"tagged\" file: $!\n";
open(R, "<$ARGV[1]") || die "can't open \"reference\" file: $!\n";
while (!eof(T) && !eof(R)) {
  $t=<T>;
  $r=<R>;
  $lno++;
  printf STDERR "%12d %12d %12d\r", $lno, $pos, $neg unless $opt_q;
  chomp($t); chomp($r);
  @ts=split(/\s+/, $t);
  @rs=split(/\s+/, $r);
  if ($#rs!=$#ts) {
    printf STDERR "Warning: different number of items in line %d\n", $lno;
  }
  for ($i=0; $i<$#rs; $i+=2) {
    # printf STDERR "%s/%s %s/%s\n", $ts[$i], $ts[$i+1], $rs[$i], $rs[$i+1];
    if ($ts[$i] ne $rs[$i]) {
      printf STDERR "Warning: words no. %d on line %d don't match\n", $i, $lno;
    }
    if ($ts[$i+1] eq $rs[$i+1]) {
      $pos++;
    } else {
      if (!defined($taghash{$ts[$i+1]})) {
	push(@tags, $ts[$i+1]);
	$taghash{$ts[$i+1]}=$#tags;
      }
      if (!defined($taghash{$rs[$i+1]})) {
	push(@tags, $rs[$i+1]);
	$taghash{$rs[$i+1]}=$#tags;
      }
      $c{$ts[$i+1]}{$rs[$i+1]}++;
      $neg++;
    }
  }
}
if (!eof(T)) {
  printf STDERR "Warning: not at end of \"tagged\" file\n";
}
if (!eof(R)) {
  printf STDERR "Warning: not at end of \"reference\" file\n";
}
close(R);
close(T);

for ($i=0; $i<=$#tags; $i++) {
  printf "%20s", $tags[$i];
  for ($j=0; $j<=$#tags; $j++) {
    printf " %4d", $c{$tags[$i]}{$tags[$j]};
  }
  printf "\n";
}
