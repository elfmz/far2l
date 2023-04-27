#!/usr/bin/env perl
use strict;

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

if (!AreSameFiles($tmp, $path)) {
	print "Updating $path\n";
	system("cp -f $tmp $path") 
}
unlink("$tmp");

## TODO: Move to dedicated .pm file
sub AreSameFiles
{
	my ($path1, $path2) = (@_);
	my ($f1, $f2);
	return undef if !open ($f1, '<', $path1);
	if (!open($f2, '<', $path2)) {
		close($f1);
		return undef;
	}
	my $out = 1;
	for (;;) {
		my $s1 = <$f1>;
		my $s2 = <$f2>;
		last if !defined($s1) && !defined($s2);
		if (!defined($s1) || !defined($s2) || $s1 ne $s2) {
			$out = undef;
			last;
		}
	}

	close($f1);
	close($f2);
	return $out;
}
