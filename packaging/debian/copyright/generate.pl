#!/usr/bin/perl

my $scandir = `git rev-parse --show-toplevel`;
chop($scandir);
die "Cannot determine git repository top-level directory" if $scandir eq '';

my (%nicknames, %licenses);

open(NICKNAMES, '<', 'nicknames') or die "nicknames: $!";
while (<NICKNAMES>) {
	chop($_);
	my ($nickname, $real) = split /\:/, trim($_);

	$nicknames{$nickname} = $real if index($nickname, '#') != 0 && $nickname ne '' && $real ne '';
}
close(NICKNAMES);


open(SKELETON, '<', 'skeleton') or die "skeleton: $!";
my @bone = ();

while (<SKELETON>) {
	chop($_);
	if (trim($_) ne '') {
		push(@bone, $_);
		next;
	}

	for (collect_bone_field('License')) {
		$licenses{$_} = 1;
	}

	my @files = collect_bone_field('Files');
	my @copyright = collect_bone_field('Copyright');
	if (scalar(@files) > 0) {
		my %commiters;
		for (@files) {
			my @subfiles = split(/ /, $_);
			for (@subfiles) {
				my $file = trim($_);
				collect_commiters($file, \%commiters) if $file ne '';
			}
		}
		append_commiters(\%commiters, \@copyright);
	}
	my $skipping = undef;
	for (@bone) {
		if ($skipping) {
			next if index($_, ' ') == 0;
			$skipping = undef;
		}
		if (index($_, 'Copyright:') == 0) {
			$skipping = 1;
			my $prefix = 'Copyright:';
			for (@copyright) {
				print "$prefix $_\n";
				$prefix = '          ';
			}

		} else {
			print("$_\n");
		}
	}
	print("\n");

	@bone = ();

}

close(SKELETON);

for $license (sort keys %licenses) {
	next if $license eq 'public-domain';
	print("License: $license\n");
	open(LICENSE, '<', "licenses/$license") or die "licenses/$license: $!";
	while(<LICENSE>) {
		chop($_);
		my $line = trim($_);
		$line = '.' if $line eq '';
		print " $line\n";
	}
	close(LICENSE);
	print("\n");
}

sub collect_commiters
{
	my ($file, $commiters) = (@_);
	my $path = "$scandir/$file";
	$path = substr($path, 0, length($path) - 2) if substr($path, -2) eq '/*';
	print STDERR "Collecting: $path\n";
	my @gitout = split /[\n\r]/, `git log --date=short --pretty=format:\"%an%x09%ad\" -- \'$path\'`;
	for $gitline (@gitout) {
		# $gitline == 'somebody	2021-09-01'
		my ($commiter, $date) = split(/\t/, trim($gitline));
		next if $commiter eq '' || $date eq '';
		$commiter = $nicknames{$commiter} if defined($nicknames{$commiter});
		if (defined(${$commiters}{$commiter})) {
			my $dates = ${$commiters}{$commiter};
			@{$dates}[0] = $date if @{$dates}[0] gt $date;
			@{$dates}[1] = $date if @{$dates}[1] lt $date;
		} else {
			${$commiters}{$commiter} = [($date, $date)];
		}
	}
}

sub append_commiters
{
	my ($commiters, $copyright) = (@_);
	my @sortable;
	# to be sorted by staring/ending dates, cuz will contain lines like:
	# 2017-03-01	2019-11-21	somebody
	for my $commiter (sort keys %{$commiters}) {
		my $dates = ${$commiters}{$commiter};
		push(@sortable, "@{$dates}[0]\t@{$dates}[1]\t$commiter");
	}
	for my $line (sort @sortable) {
		my ($date1, $date2, $commiter) = split(/\t/, $line);
		my ($year1) = split(/-/, $date1);
		my ($year2) = split(/-/, $date2);
		push(@{$copyright}, "$year1-$year2 $commiter");
	}
}

sub collect_bone_field
{
	my ($field) = (@_);
	my $collecting = undef;
	my @out = ();
	for (@bone) {
		if (index($_, ' ') == 0) {
			push(@out, trim($_)) if $collecting;

		} elsif (index($_, "$field:") == 0) {
			$collecting = 1;
			my $value = trim(substr($_, length("$field:")));
			push(@out, $value) if $value ne '';
		} else {
			$collecting = undef;
		}
	}
	return @out;
}

sub trim
{
	my ($s) = (@_);
	$s =~ s/^\s+|\s+$//g;
	return $s;
}
