#!/usr/bin/env perl

#
# Ingo Schröder
#

require "getopts.pl";

&Getopts('hqv');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h] reference a b\n" if defined($opt_h) || $#ARGV!=2;

$pos=$neg=$lno=0;
open(R, "<$ARGV[0]") || die "can't open \"reference\" file: $!\n";
open(A, "<$ARGV[1]") || die "can't open \"a\" file: $!\n";
open(B, "<$ARGV[2]") || die "can't open \"b\" file: $!\n";
while (!eof(R) && !eof(A) && !eof(B)) {
  $r=<R>;
  $a=<A>;
  $b=<B>;
  #if ($t ne $r) { printf STDERR "%s%s", $t, $r; }
  $lno++;
  printf STDERR "%12d %12d %12d\r", $lno, $pos, $neg unless defined($opt_q);
  chomp($r); chomp($b); chomp($b); 
  @rs=split(/\s+/, $r);
  @as=split(/\s+/, $a);
  @bs=split(/\s+/, $b);
  if ($#rs!=$#as || $#rs!=$#bs) {
    die "Warning: different number of items in line $lno\n";
  }
  for ($i=0; $i<$#rs; $i+=2) {
    # printf STDERR "%s/%s %s/%s\n", $ts[$i], $ts[$i+1], $rs[$i], $rs[$i+1];
    if ($rs[$i] ne $as[$i]) {
      printf STDERR "Warning: \"a\" word no. %d on line %d don't match\n", $i, $lno;
    }
    if ($rs[$i] ne $bs[$i]) {
      printf STDERR "Warning: \"b\" word no. %d on line %d don't match\n", $i, $lno;
    }
    $token++;
    if ($rs[$i+1] eq $as[$i+1]) {
      $posa++;
    } else {
      $nega++;
    }
    if ($rs[$i+1] eq $bs[$i+1]) {
      $posb++;
    } else {
      $negb++;
    }
    if ($rs[$i+1] ne $as[$i+1] && $rs[$i+1] ne $bs[$i+1]) {
      $a_and_b++;
    }
    if ($rs[$i+1] ne $as[$i+1] && $rs[$i+1] eq $bs[$i+1]) {
      $a_not_b++;
    }
    if ($rs[$i+1] eq $as[$i+1] && $rs[$i+1] ne $bs[$i+1]) {
      $b_not_a++;
    }
  }
}
if (!eof(R)) {
  printf STDERR "Warning: not at end of \"reference\" file\n";
}
if (!eof(A)) {
  printf STDERR "Warning: not at end of \"A\" file\n";
}
if (!eof(B)) {
  printf STDERR "Warning: not at end of \"B\" file\n";
}
close(B);
close(A);
close(R);

printf "accuracy A %7.3f%% %d %d\n", 100.0*"$posa.0"/("$posa.0"+"$nega.0"), $posa, $nega;
printf "accuracy B %7.3f%% %d %d\n", 100.0*"$posb.0"/("$posb.0"+"$negb.0"), $posb, $negb;
printf "comp(A,B)  %7.3f%% %d %d\n", 100.0*(1-$a_and_b/$nega), $a_and_b, $nega;
printf "comp(B,A)  %7.3f%% %d %d\n", 100.0*(1-$a_and_b/$negb), $a_and_b, $negb;

