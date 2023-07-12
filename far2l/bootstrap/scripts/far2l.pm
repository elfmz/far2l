package far2l;
use strict;

sub UpdateFromTmp
{
	my ($tmp, $path) = (@_);
	if (!AreSameFiles($tmp, $path)) {
		print "Updating $path\n";
		my @args = ("cp", "-f", $tmp, $path);
		system(@args);
	}
	unlink("$tmp");
}

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

1;
