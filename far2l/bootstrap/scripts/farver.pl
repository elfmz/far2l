#!/usr/bin/env perl
use strict;
use far2l;

my $path = shift;
my $arch = shift;
my $major = shift;
my $minor = shift;
my $patch = shift;

die 'Bad args' if !defined($path) || !defined($arch) || !defined($major) || !defined($minor) || !defined($patch);

my (@tm) = localtime(time());
my $year = $tm[5] + 1900;

my $tmp = "$path.tmp";

my $f;
open($f, '>', $tmp) or die "$tmp: $!";
printf $f "uint32_t FAR_VERSION __attribute__((used)) = 0x%04x%04x;\n", $major, $minor;
print $f "const char *FAR_BUILD __attribute__((used)) = \"$major.$minor.$patch\";\n";
print $f "const char *Copyright __attribute__((used)) = \"FAR2L, version $major.$minor.$patch $arch\\nCopyright (C) 1996-2000 Eugene Roshal, Copyright (C) 2000-2016 Far Group, Copyright (C) 2016-$year Far People\";\n";
close $f;

far2l::UpdateFromTmp($tmp, $path);
