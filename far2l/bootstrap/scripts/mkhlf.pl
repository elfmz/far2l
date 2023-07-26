#!/bin/perl
use strict;
use warnings;

use open ':std', ':encoding(UTF-8)';
use open ':encoding(UTF-8)', ':std';

my $topicFound = 0;
my $topic = "";
my $it = 0;
my %atopic = ();
my $Contents = "";

while (<>) {
  if ($_ ne "<%INDEX%>\n") {
    print $_;
  }

  if (/^\@Contents\s*$/) {
    $_ = <>;
    ($Contents) = /^\$\^#(.+)#$/;
    print $_;
  }

  if (/^\@(?! ).*$/ && $_ ne "\@Contents\n" && $_ ne "\@Index\n") {
    $topic = $_;
    $topicFound = 1;
  }

  if ($topicFound == 1 && /\S+\s+\S+/) {
    if ($_ !~ /^\$/ ||
        $_ =~ /\$\s#Warning/ ||
        $_ =~ /\$\s#Error:/ ||
        $_ =~ /\$\s#Предупреждение:/ ||
        $_ =~ /\$\s#Ошибка:/)
    {
      $topicFound = 0;
      $topic = "";
      $it--;
    }
    else {
      my ($s) = $_;
      if ($s =~ /^\$[\s\^]#(.*)#$/) {
        $s = $1;
      }
      chomp($topic);
      chomp($s);
      $atopic{$s . "~" . $topic . "\@"} = $topic;
      $topicFound = 0;
      $topic = 0;
    }
  }

  if ($_ eq "<%INDEX%>\n") {
    print "   ~" . $Contents . "~\@Contents\@\n";
    if (index(uc($ARGV),"FARENG.HLF.M4") == 0) {print "\n"}

    my $ch = " ";
    foreach my $topic2 (sort keys %atopic) {
      if ($ch ne substr($topic2, 0, 1)) {
        $ch = substr($topic2, 0, 1);
        print "\n";
      }
      print "   ~" . ltrim($topic2) . "\n";
    }
  }
}

sub ltrim {
  my $str = shift;
  $str =~ s/^\s+//;
  return $str;
}
