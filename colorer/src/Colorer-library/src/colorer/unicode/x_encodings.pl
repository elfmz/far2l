#
# x_encodings.h
# Encodings Generator

$p_warn = '/*
    This file was automatically generated from
    UNICODE 3.2.0 UnicodeData.txt base.
    Do not edit it with hands.
*/';
$p_footer = '

';

$CHARSETS = 'd:\usr\doc\unicode\PUBLIC\MAPPINGS';
$CP_TABLE_SHIFT = 7;

$defaultEncoding = "cp1251";

%csets_alias = (
  "Windows-1250" => "Cp1250",
  "Windows-1251" => "Cp1251",
  "Windows-1252" => "Cp1252",
  "KOI-8"        => "KOI8-R",       # for FAR!!!
  "ASCII"       => "8859-1",
  "US-ASCII"    => "8859-1",
  "LATIN1"      => "8859-1",
  "ISO-8859-1"  => "8859-1",
  "LATIN2"      => "8859-2",
  "ISO-8859-2"  => "8859-2",
  "LATIN3"      => "8859-3",
  "ISO-8859-3"  => "8859-3",
  "LATIN4"      => "8859-4",
  "ISO-8859-4"  => "8859-4",
  "ISO-8859-5"  => "8859-5",
  "ISO-8859-6"  => "8859-6",
  "ISO-8859-7"  => "8859-7",
  "ISO-8859-8"  => "8859-8",
  "ISO-8859-9"  => "8859-9",
);

%unicode_alias = qw(
  UTF-8     -2
  UTF-16    -3
  UTF-16LE  -3
  UTF-16BE  -4
  UTF-32    -5
  UTF-32LE  -5
  UTF-32BE  -6
);
foreach(`ls $CHARSETS`){
  chomp;
  push @flist, $_;
};


foreach $file (sort @flist){
  next if ($file !~ /^(.*)\.txt$/i);
  $csname = $1;
  open FILE, $CHARSETS."/".$file or die "can't open $file";
  foreach(<FILE>){
    next if (!/^(0x[^\s]+)\t(0x[^\s]+)/);
    $csets{$csname}{hex($1)} = hex($2);
    $crev_sets{$csname}{hex($2)} = hex($1);
  };
  $encodingIdxSize++;
  close FILE;
};

$pos = 0;
$next = 1;

$aliasEncodingIdxSize = $encodingIdxSize;

foreach $e_name (keys %unicode_alias){
  $enc = $unicode_alias{$e_name};
  $arr_idxEncodings .= "  {\"$e_name\", $enc},\n";
  $e_name =~ s/[-\s]//;
  $unicodeEncodings .= "#define ENC_$e_name ($enc)\n";
  $aliasEncodingIdxSize++;
};

foreach $cp_name (sort keys %csets){

  print "$cp_name, ";

  $arr_idxEncodings .= "  {\"$cp_name\", $pos},\n";
  foreach $al (keys %csets_alias){
    if (uc($csets_alias{$al}) eq uc($cp_name)){
      $arr_idxEncodings .= "  {\"$al\", $pos},\n";
      $aliasEncodingIdxSize++;
    };
  };
  if ($cp_name =~ /$defaultEncoding/i){
    $defencoding = $cp_name;
    $defencodingIdx = $pos;
  };
  $pos++;
  ## ansi -> ucs
  for($i=0; $i < 0x100; $i++){
    $no = $csets{$cp_name}{$i};
    $Encodings .= ($no eq''?0:$no).", ";
  };
  $Encodings .= "\n";

  ## ucs -> ansi
  $rev_arr_idxEncodings .= "\n";
  for($i=0; $i < 0x10000; $i += (1 << $CP_TABLE_SHIFT)){
    $part = subarray($i, $cp_name);
    if (!$part){
      $rev_arr_idxEncodings .= "0, ";
    }elsif($hashnext = $parthash{$part}){
      $rev_arr_idxEncodings .= "$hashnext, ";
    }else{
      $parthash{$part} = $next;
      $rev_arr_idxEncodings .= "$next, ";
      $next++;
      $rev_Encodings .= $part;
    };
  };
};

$rev_Encodings_0 = "0, " x (1 << $CP_TABLE_SHIFT);

open OUT, ">x_encodings.h";

print OUT <<x_encodings;
$p_warn

#define TO_WCHAR(encIdx, c) (arr_Encodings[0x100 * (encIdx) + (unsigned char)(c)])
#define TO_CHAR(encIdx, c) ((char)(arr_revEncodings[ ($encodingIdxSize<<(16-$CP_TABLE_SHIFT)) + (arr_revEncodings[(encIdx<<(16-$CP_TABLE_SHIFT)) + ((c)>>$CP_TABLE_SHIFT)] << $CP_TABLE_SHIFT) + ((c) & (0xFFFF>>(16-$CP_TABLE_SHIFT)))]))

static const char defEncoding[16] = "$defencoding";
static const int defEncodingIdx = $defencodingIdx;

static struct{char name[16]; int pos;} arr_idxEncodings[] = {
$arr_idxEncodings};

static const int encNamesNum   = $encodingIdxSize;
static const int encAliasesNum = $aliasEncodingIdxSize;

static wchar arr_Encodings[] = {
$Encodings
};

static unsigned char arr_revEncodings[] = {
$rev_arr_idxEncodings

$rev_Encodings_0

$rev_Encodings};

$p_footer
x_encodings


close OUT;


sub subarray{
  my $pos = shift @_;
  my $csname = shift @_;
  my $list;
  my $e = 1;
  for($t = $pos; $t < $pos + (1 << $CP_TABLE_SHIFT); $t++){
    $res = $crev_sets{$csname}{$t};
    $e = 0 if ($res ne '');
    $res = 0 if ($res eq '');
    $list .= $res.", ";
  };
  return 0 if ($e);
  return $list."\n";
};
# ***** BEGIN LICENSE BLOCK *****
# Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program; If not, see <http://www.gnu.org/licenses/>.
# ***** END LICENSE BLOCK *****
