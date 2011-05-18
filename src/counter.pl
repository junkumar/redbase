#! /opt/local/bin/perl -w

use strict;

my $ret;

while(<STDIN>) {
  chomp;
  print "$_\n";
  if (m/^(\d+) tuple/) {
    $ret = $1;
  }
}

if(defined $ret) {
  exit($ret);
}
else {
  exit(0);
}
