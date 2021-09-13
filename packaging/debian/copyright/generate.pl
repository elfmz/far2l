#!/usr/bin/perl

my @skeleton = ();
my $pathes = [()];
my %aliases;

open(FH, '<', 'skeleton') or die "skeleton: $!";
while (<FH>) {
	chop($_);
	my $tr = trim($_);

	next if $tr eq '' || index($tr, '#') == 0;

	if (index($_, "\t") == 0) {
		my ($license, $author) = split /\:/, $tr;
		push(@skeleton, [($pathes, file2string("licenses/$license"), file2string("authors/$author"))]);
		$pathes = [()];

	} else {
		push(@{$pathes}, $tr);
	}
}
close(FH);

open(FH, '<', 'authors/ALIASES') or die "authors/ALIASES: $!";
while (<FH>) {
	chop($_);
	next if index($_, '#') == 0 || $_ eq '';

	my ($alias, $real) = split /\:/, trim($_);
	$aliases{$alias} = $real if $alias ne '' && $real ne '';
}
close(FH);

my $scandir = `git rev-parse --show-toplevel`;
chop($scandir);
die "Cannot determine git repository top-level directory" if $scandir eq '';

for $bone (@skeleton) {
	my %authors;
	for $path (@{@{$bone}[0]}) {
		print STDERR "Scanning: $path\n";
		my @gitout = split /[\n\r]/, `git log --date=short --pretty=format:\"%an%x09%ad\" -- \'$scandir/$path\'`;
		for $gitline (@gitout) {
			# $gitline == 'somebody	2021-09-01'
			my ($author, $date) = split(/\t/, trim($gitline));
			next if $author eq '' || $date eq '';
			$author = $aliases{$author} if defined($aliases{$author});
			if (defined($authors{$author})) {
				my $dates = $authors{$author};
				@{$dates}[0] = $date if @{$dates}[0] gt $date;
				@{$dates}[1] = $date if @{$dates}[1] lt $date;
			} else {
				$authors{$author} = [($date, $date)];
			}
		}
	}
	my @sortable;
	# to be sorted by staring/ending dates, cuz will contain lines like:
	# 2017-03-01	2019-11-21	somebody
	for my $author (sort keys %authors) {
		my $dates = $authors{$author};
		push(@sortable, "@{$dates}[0]\t@{$dates}[1]\t$author");
	}
	for my $line (sort @sortable) {
		my ($date1, $date2, $author) = split(/\t/, $line);
		my ($year1) = split(/-/, $date1);
		my ($year2) = split(/-/, $date2);
		@{$bone}[2].= "$year1-$year2 $author\n";
	}
}


print "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/\n";
print "Upstream-Name: far2l\n";
print "Upstream-Contact: Yes\n";
print "Source: https://github.com/elfmz/far2l\n";
for $bone (@skeleton) {
	print "Files:";
	for $path (@{@{$bone}[0]}) {
		my $xpath = substr($path, 1);
		$xpath.= '*' if substr($path, -1) eq '/';
		print " $xpath";
	}
	print "\n";

	my $cr_prefix = "Copyright:";
	for my $author (split /[\n\r]/, @{$bone}[2]) {
		print "$cr_prefix $author\n";
		$cr_prefix = "          ";
	}

	print "License: ";
	print @{$bone}[1]; # license
	print "\n";
}

sub trim
{
	my ($s) = (@_);
	$s =~ s/^\s+|\s+$//g;
	return $s;
}

sub file2string
{
	my ($f) = (@_);
	my $out = '';

	open(FX, '<', $f) or die "$f: $!";
	while (<FX>) {
		$out.= $_;
	}
	close(FX);

	return $out;
}

