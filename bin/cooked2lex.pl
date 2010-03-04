#!/usr/bin/env perl

#
# Ingo Schr�der
#

require "getopts.pl";

&Getopts('ch');

$cmd=$0;
$cmd=~s/(.*\/)*//;

die "Usage: $cmd [-c] [-h]\n" if defined($opt_h) || $#ARGV!=-1;

$now=$lno=0;
while ($l=<STDIN>) {
  $lno++;
  printf STDERR "%d sentences\r", $lno if $lno%100==0;
  chomp($l);
  @ls=split(/\s+/, $l);
  for ($i=0; $i<$#ls; $i+=2) {
    $now++;
    $wc{$ls[$i]}++;
    $tc{$ls[$i+1]}++;
    $wtc{$ls[$i]}{$ls[$i+1]}++;
  }
}
printf STDERR "%d sentences\n", $lno;

$maxx=-1;
@words=sort keys(%wc);
#$wa=0.0;
foreach $w (@words) {
  printf "%s", $w;
  printf " %d", $wc{$w} if defined($opt_c);
  my $x=0;
  my $wc=$wc{$w};
 TAG:
  foreach $t (sort { $wtc{$w}{$b} <=> $wtc{$w}{$a}  } keys(%tc)) {
    next TAG unless defined($wtc{$w}{$t});
    #$wa+=$wtc{$w}{$t}/$wc{$w};
    $x++;
    printf " %s %d", $t, $wtc{$w}{$t};
    $wc-=$wtc{$w}{$t};
  }
  die "ERROR: inconsistency for $w\n" if $wc!=0;
  $tac_token[$x]+=$wc{$w};
  $tac_type[$x]++;
  $maxx= $x if $x>$maxx;
  printf "\n";
}

@tags=keys %tc;
printf STDERR "%d tags %d types %d tokens\n", $#tags+1, $#words+1, $now;
undef @tags;
$ambiguity=0.0;
for ($i=1; $i<=$maxx; $i++) {
  printf STDERR "%3d %9d %7.3f%% %9d %7.3f%% \n",
    $i, $tac_type[$i], 100*$tac_type[$i]/($#words+1), $tac_token[$i], 100*$tac_token[$i]/$now;
  $ambiguity+= $tac_token[$i] * $i;
}
$ambiguity/=$now;
printf STDERR "Mean ambiguity A=%f\n", $ambiguity;

#printf STDERR "Weighted ambiguity WA=%f\n", $wa;

$entropy=0.0;
foreach $t (keys %tc) {
  my $p=$tc{$t}/$now;
  $entropy-=  $p * log($p)/log(2);
}
printf STDERR "\nEntropy H(p)=%f\n", $entropy;
