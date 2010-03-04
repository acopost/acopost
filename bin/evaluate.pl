#!/usr/bin/env perl

#
# Ingo Schr�der
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

use FileHandle;

require "getopts.pl";

&Getopts('hil:v');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-h] [[-i] -l lexicon] [-v] reference t1 ...\n"
  if defined($opt_h) || defined($opt_h) || $#ARGV<1;

if (defined($opt_l)) {
  $nle=0;
  open(F, "<$opt_l") || die "can't open lexicon \"$opt_l\": $!\n";
  while ($l=<F>) {
    if ($l=~m/^(\S+)\s+(.+)$/) {
      my $w=$1;
      $w=~tr/A-Z���/a-z���/ if defined($opt_i);
      my @vs=split(/\s+/, $2);
      $lexicon{$w}=$vs[0];
      for (my $i=0; $i<=$#vs; $i+=2) {
	$tc{$vs[$i]}+=$vs[$i+1];
      }
      $nle++;
    }
  }
  close(F);
  printf "%d lexicon entries read\n", $nle if defined($opt_v);
  @tags=sort { $tc{$b} <=> $tc{$a} } keys %tc;
  $mft=$tags[0];
  printf "most frequent tag %s\n", $mft if defined($opt_v);
}

open(R, "<$ARGV[0]") || die "can't open reference file \"$ARGV[0]\": $!\n";
for ($i=1; $i<=$#ARGV; $i++) {
  my $fh=new FileHandle;
  $fh->open("<$ARGV[$i]") || die "can't open tagged file \"$ARGV[$i]\": $!\n";
  push(@fns, $ARGV[$i]);
  push(@fhs, $fh);
  push(@pos, 0);
  push(@lpos, 0);
  push(@neg, 0);
  push(@lneg, 0);
}

$lno=0;
while ($r=<R>) {
  chomp($r);
  $lno++;
  printf STDERR "%12d sentences read\r", $lno if defined($opt_v);
  @rs=split(/\s+/, $r);
  for ($i=0; $i<=$#fhs; $i++) {
    my $bnoe=$noe=0;
    $t=$fhs[$i]->getline();
    @ts=split(/\s+/, $t);
    die "$fns[$i]:$lno: different number of words $#rs != $#ts\n" if $#rs!=$#ts;
    for ($j=0; $j<$#rs; $j+=2) {
      my $pos=$j/2;
      if (defined($opt_i)) {
	$rs[$j]=~tr/A-Z���/a-z���/;
	$ts[$j]=~tr/A-Z���/a-z���/;
      }
      die "$fns[$i]:$lno: different words at pos $pos\n" if $rs[$j] ne $ts[$j];

      if ($i==0) {
	if (defined($lexicon{$rs[$j]})) {
	  $known++;
	  if ($lexicon{$rs[$j]} eq $rs[$j+1]) {
	    $bkpos++;
	  } else {
	    $bkneg++;
	    $bnoe++;
	  }
	} else {
	  $unknown++;
	  if ($mft eq $rs[$j+1]) {
	    $bupos++;
	  } else {
	    $buneg++;
	    $bnoe++;
	  }
	}
      }
      if ($rs[$j+1] eq $ts[$j+1]) {
	$pos[$i]++;
	$lpos[$i]++ if defined($lexicon{$rs[$j]});
      } else {
	$noe++;
	$neg[$i]++;
	$lneg[$i]++ if defined($lexicon{$rs[$j]});
      }
    }
    $bsc++ if $i==0 && $bnoe==0;
    $sc[$i]++ if $noe==0;
  }
}
printf STDERR "\n" if defined($opt_v);
$nos=$lno;

printf "%d sentences\n", $nos;
printf "%d words == %d (%7.3f%%) known %d (%7.3f%%) unknown\n",
  $known+$unknown, $known, 100*$known/($known+$unknown), $unknown, 100*$unknown/($known+$unknown)
  if defined($opt_v);

for ($i=0; $i<=$#fhs; $i++) {
  die "not at end of file \"$fns[$i]\"\n" if !eof($fh[$i]);
  close($fh[$i]);
}
close(R);

for ($i=0; $i<=$#fhs; $i++) {
  my $p=$pos[$i];
  my $lp=$lpos[$i];
  my $n=$neg[$i];
  my $ln=$lneg[$i];
  if (defined($opt_l)) {
    if ($i==0 && defined($opt_v)) {
      printf "%20s|%26s|%26s\n", "", "Sentence", "Word";
      printf "%20s|%26s|%26s %26s %26s\n", "Name", "all", "all", "known", "unknown";
      printf "%s\n", "-" x 128;
      printf "%20s|%8d %8d %7.3f%%|%8d %8d %7.3f%% %8d %8d %7.3f%% %8d %8d %7.3f%%\n",
	"baseline", $bsc, $nos-$bsc, 100*$bsc/$nos,
	  $bkpos+$bupos, $bkneg+$buneg, 100*($bkpos+$bupos)/($bkpos+$bupos+$bkneg+$buneg),
	    $bkpos, $bkneg, 100*$bkpos/($bkpos+$bkneg),
	      $bupos, $buneg, 100*$bupos/($bupos+$buneg);
    }

    printf "%20s|%8d %8d %7.3f%%|%8d %8d %7.3f%% %8d %8d %7.3f%% %8d %8d %7.3f%%\n",
      $fns[$i], $sc[$i], $nos-$sc[$i], 100*$sc[$i]/$nos,
	$p, $n, 100.0*$p/($p+$n),
	  $lp, $ln, 100.0*$lp/($lp+$ln),
	    ($p-$lp), ($n-$ln), 100.0*($p-$lp)/($p-$lp+$n-$ln);
  } else {
    printf "%20s %8d %8d %7.3f%%\n",
      $fns[$i], $p, $n, 100.0*$p/($p+$n);
  }
}
