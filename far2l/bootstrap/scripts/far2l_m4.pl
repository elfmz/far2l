#!/usr/bin/perl
use warnings;
use strict;

# Значения для замены

my $major = shift;
my $minor = shift;
my $patch = shift;
my $arch = shift;
my $full_version = "$major.$minor.$patch $arch";

# Получаем текущий год
my $current_year = (localtime)[5] + 1900;
my $copyright_years = "2016-$current_year";

# Открываем файл и читаем его содержимое
my $first_line = 1;
while (my $line = <STDIN>) {
    if ($first_line) {
        $first_line = 0;
        next if $line =~ /m4_include\(`farversion\.m4'\)m4_dnl/;
    }
    $line =~ s/FULLVERSIONNOBRACES/$full_version/g;
    $line =~ s/MAJOR/$major/g;
    $line =~ s/MINOR/$minor/g;
    $line =~ s/PATCH/$patch/g;
    $line =~ s/COPYRIGHTYEARS/$copyright_years/g;

    
    # Удаление символов ` и ' только из строк,
    # начинающихся с ` или содержащих `#-#', '.' (в кавычках) или 'Perl regexp`s'
    #if ($line =~ /^`/ || $line =~ /`#-#'/ || $line =~ /'\.'/ || $line =~ /Perl regexp`s/) {
    if ($line =~ /^`/ || $line =~ /`#-#'/ || $line =~ /Perl regexp`s/) {
        $line =~ s/[`']//g;
    }

    print "$line";
}
