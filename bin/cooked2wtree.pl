#!/usr/bin/env perl

#
# Ingo Schröder, schroeder@informatik.uni-hamburg.de
#
# see http://nats-www.informatik.uni-hamburg.de/~ingo/icopost/
#

#use strict;

require "getopts.pl";

$cmd=$0;
$cmd=~s/(.*\/)*//;

printf "##\n## Call (quotes inserted): %s", $cmd;
foreach my $o (@ARGV) {
  printf " \"%s\"", $o;
}
printf "\n";

&Getopts('a:b:de:i:dhr:w:');

$minc=defined($opt_a) ? $opt_a : -1;
$maxc=defined($opt_b) ? $opt_b : -1;
$debug=defined($opt_d) || defined($opt_d) ? 1 : 0;
$exclude=defined($opt_e) ? $opt_e : "";
$include=defined($opt_i) ? $opt_i : "";
$rwt=defined($opt_r) ? $opt_r : 0;
$mfw=defined($opt_w) ? $opt_w : 100;


die "Usage: $cmd [-a min] [-b max] [-d] [-h] [-w mfw] featurefile\n"
  if defined($opt_h) || defined($opt_h) || $#ARGV!=0;

my $user = getlogin() eq "" ? $< : getlogin();
@pw=getpwnam($user);
printf "## User: %s\n", $pw[6];

my($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime(time);

printf "##\n## Date/Time: %2.2u.%2.2u.%4.4u %2.2u:%2.2u\n##\n",
  $mday, $mon+1, 1900+$year, $hour, $min;

printf "## Minimal word count: %d\n", $minc;
printf "## Maximal word count: %d\n", $maxc;
printf "## Rare word threshold: %d\n", $rwt;
printf "## Word rank threshold: %d\n", $mfw;

if ($exclude ne "") {
  printf "## Tags excluded:";
  open(F, "< $exclude") || die "can't open exclude file \"$exclude\": $!\n";
  while ($l=<F>) {
    chomp($l);
    $excluded{$l}=1;
    printf " %s", $l;
  }
  printf "\n";
  close(F);
}
if ($include ne "") {
  printf "## Tags included:";
  open(F, "< $include") || die "can't open include file \"$include\": $1\n";
  while ($l=<F>) {
    chomp($l);
    $included{$l}=1;
    printf " %s", $l;
  }
  printf "\n";
  close(F);
}

open(F, "<$ARGV[0]") || die "can't open feature file \"$ARGV[0]\": $!\n";
while ($l=<F>) {
  chomp($l);
  $l=~s/\s//g;
  push(@fs, $l) if $l=~m/[A-Z]/;
}
close(F);

printf STDERR "No. of features: %d (from \"%s\")\n", $#fs+1, $ARGV[0];
printf "## No. of features: %d (from \"%s\")\n", $#fs+1, $ARGV[0];

$log_of_2=log(2);

$now=$lno=0;
while ($l=<STDIN>) {
  $lno++;
  printf STDERR "%d\r", $lno
    if $debug>0 && $lno%100==0;
  chomp($l);
  push(@s, $l);
  my @ls=split(/\s+/, $l);
  for ($i=0; $i<$#ls; $i+=2) {
    $now++;
    $wc{$ls[$i]}++;
    $tc{$ls[$i+1]}++;
    $wtc{$ls[$i]}{$ls[$i+1]}++;
  }
}

printf STDERR "No. of sentences: %d\n", $lno;
printf "## No. of sentences: %d\n", $lno;
printf STDERR "No of words: %d\n", $now;
printf "## No of words: %d\n", $now;

# determine most frequent words threshold
@ws=keys(%wc);
@sws=sort { $wc{$b} <=> $wc{$a} } @ws;

printf STDERR "Most frequent words:\n";
for (my $i=0; $i<5 && $i<=$#sws; $i++) {
  printf STDERR "\t%8d\t\"%s\"\n", $wc{$sws[$i]}, $sws[$i];
}

# Words must be more frequent than $mfwt to be considered for WORD[]
if ($mfw>$#sws) {
  printf STDERR "WARNING: Less than %d words (namely %d) found!\n", $mfw, $#sws+1;
  printf "## WARNING: Less than %d words (namely %d) found!\n", $mfw, $#sws+1;
  $mfwt=0;
} else {
  printf STDERR "Word at rank %d: \"%s\" (%d occurances)\n", $mfw, $sws[$mfw], $wc{$sws[$mfw]};
  printf "## Word at rank %d: \"%s\" (%d occurances)\n", $mfw, $sws[$mfw], $wc{$sws[$mfw]};
  $mfwt=$wc{$sws[$mfw]};
}
printf STDERR "Frequent word threshold: %d\n", $mfwt;
printf "## Frequent word threshold: %d\n", $mfwt;

undef @sws;

$H=0;
foreach my $t (keys %tc) {
  my $p=$tc{$t}/$now;
  $H+=-$p*&Log2($p);
}
printf STDERR "Entropy: %f\n", $H;
printf "## Entropy: %f\n", $H;

# compute tag classes
foreach $w (@ws) {
  if ($wc{$w}<$rwt) {
    # Not a good idea.
    $class{$w}="*RARE*";
  } else {
    my @mytags=();
    foreach $t (keys(%tc)) {
      push(@mytags, $t) if defined($wtc{$w}{$t});
    }
    my @smt=sort { $wtc{$w}{$b} <=> $wtc{$w}{$a} } @mytags;
    my $class="";
    for (my $i=0; $i<=$#smt; $i++) {
      $class.="*" . $smt[$i];
    }
    $class.="*";
    $class{$w}=$class;
  }
}

# not needed any longer
undef @ws;

$rn=0;
for (my $i=0; $i<=$#s; $i++) {
  my $l=$s[$i];
  my @ls=split(/\s+/, $l);

 WORD:
  for (my $j=0; $j<=$#ls; $j+=2) {
    my $w=$ls[$j];
    my $wc=$wc{$w};
    next WORD if $minc>=0 && $wc<$minc;
    next WORD if $maxc>=0 && $wc>$maxc;
    my $t=$ls[$j+1];
    next WORD if $exclude ne "" && defined($excluded{$t});
    next WORD if $include ne "" && !defined($included{$t});

    #printf "%s", $t;
    for ($k=0; $k<=$#fs; $k++) {
      my $f=$fs[$k];
      if ($f=~m/^TAG\[(-?\d+)\]$/) {
	my $rp=$1;
	die "TAG only known to the left\n" if $rp>=0;
	my $pos=$j+2*$rp+1;
	$tmp= ($pos<0 || $pos>$#ls) ? "*BOUNDARY*" : $ls[$pos];
      } elsif ($f=~m/^CLASS\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	$tmp= ($pos<0 || $pos>$#ls) ? "*BOUNDARY*" : $class{$ls[$pos]};
      } elsif ($f=~m/^WORD\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	$tmp= ($pos<0 || $pos>$#ls) ? "*BOUNDARY*" : ($wc{$ls[$pos]}>$mfwt ? $ls[$pos] : "*RARE*");	
      } elsif ($f=~m/^LETTER\[(-?\d+),(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	my $index=$2;
	$index-- if $index>0;
	if ($pos<0 || $pos>$#ls) { $tmp="*BOUNDARY*"; }
	else {
	  my $w=$ls[$pos];
	  my $wl=length($w);
	  if ($index>=$wl || -$index>$wl) {
	    $tmp="*BL*";
	  } else {
	    my $letter=substr($w, $index, 1);
	    $tmp= $letter eq "" ? "*NONE*" : $letter;
	  }
	}
      } elsif ($f=~m/^CAP\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	if ($pos<0 || $pos>$#ls) { $tmp="*BOUNDARY*"; }
	else {
	  my $w=$ls[$pos];
	  $tmp= $w=~m/^[A-ZÖÜÄ]/ ? $w=~m/^[A-ZÖÜÄ]+$/ ? 2 : 1 : 0;
	}
      } elsif ($f=~m/^HYPHEN\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	if ($pos<0 || $pos>$#ls) { $tmp="*BOUNDARY*"; }
	else {
	  my $w=$ls[$pos];
	  $tmp= $w=~m/\-/ ? 1 : 0;
	}
      } elsif ($f=~m/^NUMBER\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	if ($pos<0 || $pos>$#ls) { $tmp="*BOUNDARY*"; }
	else {
	  my $w=$ls[$pos];
	  $tmp= $w=~m/\d/ ? $w=~m/^[\d,\.]+$/ ? $w=~m/^\d+$/ ? 3 : 2 : 1 : 0;
	}
      } elsif ($f=~m/^INTER\[(-?\d+)\]$/) {
	my $rp=$1;
	my $pos=$j+2*$rp;
	if ($pos<0 || $pos>$#ls) { $tmp="*BOUNDARY*"; }
	else {
	  my $w=$ls[$pos];
	  $tmp= $w=~m/^[,\.\;\?\!\:]+$/ ? 1 : 0;
	}
      } else {
	die "unknown feature description \"$f\"\n";
      }
      #printf " %s", $tmp;
      $c2{$k}{$tmp}++;
      $c3{$k}{$tmp}{$t}++;
      $v[$rn][$k]=$tmp;
    }
    $t[$rn]=$t;
    $rn++;
    #printf "\n";
  }
}

printf STDERR "Features:\n";
printf "## Features:\n";
for ($k=0; $k<=$#fs; $k++) {
  $H[$k]=0;
  $S[$k]=0;
  foreach $v (keys %{ $c2{$k} }) {
    my $sp=$c2{$k}{$v}/$now;
    $S[$k]+=-$sp*&Log2($sp);

    my $Hv=0.0;
  T:
    foreach my $t (keys %tc) {
      next T if !defined($c3{$k}{$v}{$t});
      my $p=$c3{$k}{$v}{$t}/$c2{$k}{$v};
      $Hv+=-$p*&Log2($p);
    }
    #printf STDERR "Hv for %s is %4.3e\n", $v, $Hv;
    $H[$k]+=$Hv * $c2{$k}{$v}/$now;
  }
  $IG[$k]=$H - $H[$k];
  $GR[$k]=$IG[$k]/$S[$k];
  printf STDERR "  %d %20s H==%5.3f IG==%5.3f S==%5.3f GR==%5.3f\n",
    $k, $fs[$k], $H[$k], $IG[$k], $S[$k], $GR[$k];
  printf "##  %d %20s H==%5.3f IG==%5.3f S==%5.3f GR==%5.3f\n",
    $k, $fs[$k], $H[$k], $IG[$k], $S[$k], $GR[$k];
}

printf "\n## No more additional empty lines or comments after the next line!\n";
printf "\n";

printf STDERR "Permutation:";
@sorter=sort { $GR[$b] <=> $GR[$a] } (0 ... $#fs);
for ($i=0; $i<=$#sorter; $i++) {
  my $ri=$sorter[$i];
  printf STDERR " %d", $ri;
  printf "%s %10.9e\n", $fs[$ri], $GR[$ri];
}
printf STDERR "\n";

%tree=();

for (my $i=0; $i<=$#t; $i++) {
  &MoveToTree(\%tree, $i, -1);
}

&PrintTree(\%tree, 0);
printf "\n";

exit 0;

# ------------------------------------------------------------
sub PrintTree () {
  my($t, $i)=@_;
  my @keys=keys %{ $t->{'values'} };

  if ($#keys>=0) {
    printf "\n";
    foreach my $k (@keys) {
      print "\t" x $i;
      printf "%s", $k;
      &PrintTree($t->{'values'}{$k}, $i+1);
    }
  } else {
    printf "\t";
    foreach my $tag (sort { $t->{'tagcount'}{$b} <=> $t->{'tagcount'}{$a} } keys %{ $t->{'tagcount'} }) {
      printf "\t%s %d", $tag, $t->{'tagcount'}{$tag};
    }
    printf "\n";
  }
}

# ------------------------------------------------------------
sub MoveToTree () {
  my($tree, $s, $f)=@_;
  my $tag=$t[$s];

  $tree->{'tagcount'}{$tag}++;
  $f++;

  if ($f<=$#fs) {
    my $rf=$sorter[$f];
    my $v=$v[$s][$rf];

    &MoveToTree(\%{$tree->{'values'}{$v}}, $s, $f)
  }
}

# ------------------------------------------------------------
sub Log2 () {
  my($x)=@_;

  return log($x)/$log_of_2;
}
