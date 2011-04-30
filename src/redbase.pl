#! /opt/local/bin/perl -w

use strict;
use IPC::Open2;

use Term::ReadLine;
my $term = Term::ReadLine->new('Redbase Wrapper');
my $prompt = "redbase>> ";
my $OUT = $term->OUT || \*STDOUT;

#my $pid = open2(\*CHLD_OUT, \*CHLD_IN, "./redbase test");
#my $pid = open2(\*CHLD_OUT, \*CHLD_IN, "cat -u");


# while ( defined ($_ = $term->readline($prompt)) ) {
#   print CHLD_IN "$_\n";
#   my $res = <CHLD_OUT>;
#   warn $@ if $@;
#   print $OUT $res, "\n" unless $@;
#   $term->addhistory($_) if /\S/;
# }

use Expect::Simple;

my $obj = new Expect::Simple 
      { Cmd => [ 'redbase', 'test' ],
          Prompt => [-re => 'REDBASE\s+>>\s+'],
            DisconnectCmd => 'exit;',
              Verbose => 3,
                Debug => 3,
                  Timeout => 100
                };

$obj->send("help;");
print "matched  " . $obj->match_str();

my $res;
($res = $obj->before) =~ tr/\r//d;

print $res;

while ( defined ($_ = $term->readline($prompt)) ) {
  $obj->send($_);

  print $obj->before;
  print $obj->after;
  print $obj->match_str, "\n";
  print $obj->match_idx, "\n";
  print $obj->error_expect;
  print $obj->error;


  my $res = $obj->after;
  warn $@ if $@;
  print $OUT $res, "\n" unless $@;
  $term->addhistory($_) if /\S/;
}


