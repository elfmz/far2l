#
# x_charcategory.h and x_tables.h
# generator
#

$UNICODE = 'd:/usr/doc/unicode/PUBLIC/UNIDATA/UnicodeData.txt';
$TABLE_SHIFT = 4;

$char_prop_format = '(($v_offs << 16) + ($v_tc << 15) + ($v_mirrored<<14) + ($v_isnumber<<13) + ($v_comb_class<<5) + $v_ctype)';
$char_prop_format1 = '($v_number?"(float)":"").($v_number)';

$p_warn = '/*
    This file was automatically generated from
    UNICODE 3.2.0 UnicodeData.txt base.
    Do not edit it with hands.
*/';
$p_footer = '

';


open UN, $UNICODE;
@UN = <UN>;
close UN;

foreach(@UN){
  chomp;
  my @f = split/;/;

  next if (hex($f[0]) >= 0x10000);
  next if ($f[1] =~ /^<Private Use|<The Private Use Area/);
  $ref[hex($f[0])] = \@f;
  #CJK, Hangul united blocks
  if ($f[1] =~ /, Last>/){
    my $pos = hex($f[0]) - 1;
    print "range - $f[1] - $pos:";
    while($ref[$pos] eq ''){
      $ref[$pos] = \@f;
      $pos--;
    };
    print "$pos\n";
  };
  $categ_list{$f[2]} = 1;
  $bidi_list{$f[4]} = 1;
};


########################### making categories ##############################
$cn = 0;
$p_category_enum = "CHAR_CATEGORY_Cn = $cn, // unassigned\n";
$p_enum_names = "\"Cn\", ";
$p_enum_names_w = "L\"Cn\", ";
$h_category_enum{"Cn"} = $cn++;
foreach(sort keys %categ_list){
  $p_category_enum .= "CHAR_CATEGORY_"."$_ = ".($cn).",\n";
  $p_enum_names .= "\"$_\", ";
  $p_enum_names_w .= "L\"$_\", ";
  $h_category_enum{$_} = $cn++;
};
$cn = 0;
foreach(sort keys %bidi_list){
  $p_bidi_enum .= "BIDI_CLASS_"."$_ = ".($cn).",\n";
  $p_bidi_names .= "\"$_\", ";
  $p_bidi_names_w .= "L\"$_\", ";
  $h_bidi_enum{$_} = $cn++;
};

####### Character Categories bit two-stage tables
for($i = 0; $i < 0x10000; $i++){
  next if (!$ref[$i][0]);
  $categ_arr{$ref[$i][2]} = [] if (!$categ_arr{$ref[$i][2]});
  $cprop = $categ_arr{$ref[$i][2]};
  $index = $$cprop[0]->[$i>>8];
  if (!$index){
    $index = @$cprop;
    $$cprop[0]->[$i>>8] = $index;
  };
  $$cprop[$index]->[($i&0xFF)>>5] |= (1 << ($i&0x1f));
};
$global_index = 0;
$categoryClasses .= "\n\n// empty node\n 0,0,0,0, 0,0,0,0,\n";
foreach (sort keys %categ_arr){
  $categoryClasses .= "\n\n// CHAR_CATEGORY_$_\n";
  $categoryClassesIdx .= "\n// CHAR_CATEGORY_$_\n";
  $cprop = $categ_arr{$_};
  for ($e = 0; $e < @$cprop; $e++){
    $el = $$cprop[$e];
    for($i = 0; $i < ($e?8:0x100); $i++){
      my $val = ($$el[$i]?$$el[$i]:0);
      if ($e){
        $categoryClasses .= "$val, ";
      }else{
        if ($val <= $global_index && $val){
          $global_index++;
          $val = $global_index;
        }elsif($val){
          $global_index = $val;
        };
        $val *=8;
        $categoryClassesIdx .= "$val, ";
      };
    };
    $categoryClasses .= "\n" if ($e);
    $categoryClassesIdx .= "\n" unless ($e);
  };
};

print "\ngeneral properties..";
$next = 1;
for ($i = 0; $i < 0x10000; $i += (1 << $TABLE_SHIFT)){
  print "." if ($i % 0x500 == 0);
  $part = subarray($i, $char_prop_format);
  if (!$part){
    $p_abase .= "0, ";
  }elsif($hashnext = $parthash{$part}){
    $p_abase .= "$hashnext, ";
  }else{
    $parthash{$part} = $next;
    $p_abase .= "$next, ";
    $next++;
    $p_avar .= $part;
  };
};
$p_avar0 = "0, " x (1 << $TABLE_SHIFT);
$p_ssize =  2*(0x10000 >> $TABLE_SHIFT)  +  $next*(1 << $TABLE_SHIFT)*4;

print "\nnumber values..";
$next = 1;
for ($i = 0; $i < 0x10000; $i += (1 << $TABLE_SHIFT)){
  print "." if ($i % 0x500 == 0);
  $part = subarray($i, $char_prop_format1);
  if (!$part){
    $p_a1base .= "0, ";
  }elsif($hashnext = $part1hash{$part}){
    $p_a1base .= "$hashnext, ";
  }else{
    $part1hash{$part} = $next;
    $p_a1base .= "$next, ";
    $next++;
    $p_a1var .= $part;
  };
};
$p_ssize1 =  2*(0x10000 >> $TABLE_SHIFT)  +  $next*(1 << $TABLE_SHIFT)*4;

#####################################################################

open OUT, ">x_charcategory.h";
print OUT <<xcharclass;
$p_warn

// all associations except CHAR_CATEGORY_Cn = 0
// could be changed with changes in UNICODE database
enum ECharCategory{
$p_category_enum
CHAR_CATEGORY_LAST
};

enum ECharBidi{
$p_bidi_enum
BIDI_CLASS_LAST
};

$p_footer
xcharclass
close OUT;

#####################################################################

open OUT, ">x_charcategory_names.h";
print OUT <<xcharclass;
$p_warn

// ansi class names
static char char_category_names[][3] = { $p_enum_names };

// ansi class names
// UNUSED: static char char_bidi_names[][4] = { $p_bidi_names };

/*
// unc class names
static wchar wchar_category_names[][3] = { $p_enum_names_w };

// unc class names
static wchar wchar_bidi_names[][4] = { $p_bidi_names_w };
*/

$p_footer
xcharclass
close OUT;

#####################################################################

open OUT, ">x_charcategory2.h";
print OUT <<xcharcategory2;
$p_warn

// char categories bit two stage tables index
static unsigned short arr_idxCharCategoryIdx[] = {
$categoryClassesIdx
};

// char categories bit two stage tables
static unsigned int arr_idxCharCategory[] = {
$categoryClasses
};

$p_footer
xcharcategory2

#####################################################################

open OUT, ">x_defines.h";
print OUT <<xdefines;
$p_warn

// macroses to access char properties
#define CHAR_PROP(c) (arr_CharInfo[ (arr_idxCharInfo[(c)>>$TABLE_SHIFT]<<$TABLE_SHIFT) + ((c) & (0xFFFF>>(16-$TABLE_SHIFT)))])
#define CHAR_PROP2(c) (arr_CharInfo2[ (arr_idxCharInfo2[(c)>>$TABLE_SHIFT]<<$TABLE_SHIFT) + ((c) & (0xFFFF>>(16-$TABLE_SHIFT)))])
#define TITLE_CASE(c) ((c) & (1 << 15))
#define CHAR_CATEGORY(c) ((c) & 0x1F)
#define MIRRORED(c) ((c) & (1 << 14))
#define NUMBER(c) ((c) & (1 << 13))
#define COMBINING_CLASS(c) (((c) >> 5) & 0xFF)

$p_footer
xdefines
close OUT;

#####################################################################

open OUT, ">x_tables.h";
print OUT <<xtables;
$p_warn

// index table
static unsigned short arr_idxCharInfo[] = {
$p_abase
};
// referring tables
static unsigned int arr_CharInfo[] = {
$p_avar0
$p_avar
};
// total structure size: $p_ssize



// number index table
static unsigned short arr_idxCharInfo2[] = {
$p_a1base
};
// number referring tables
static float arr_CharInfo2[] = {
$p_avar0
$p_a1var
};
// total structure size: $p_ssize1

$p_footer
xtables

close OUT;

#####################################################################

#####################################################################

sub subarray{
  my $pos = shift @_;
  my $format = shift @_;
  my $list;
  my $e = 1;
  for($t = $pos; $t < $pos + (1 << $TABLE_SHIFT); $t++){
    my $line = $ref[$t];
    $res = "";
    if ($line && $$line[0] ne ''){
      my $v_ctype = $h_category_enum{$$line[2]};
      my $v_comb_class = $$line[3];
      my $v_bidi_type = $h_bidi_enum{$$line[4]};
      my $v_tc = 0;
      $v_tc = 1 if ($$line[14] ne '' && $$line[14] ne $$line[12]); # titlecase
      my $v_offs = 0;
      $v_offs = hex($$line[0]) - hex($$line[12]) if ($$line[12] ne ''); # uppercase
      $v_offs = hex($$line[0]) - hex($$line[13]) if ($$line[13] ne ''); # lowercase
      my $v_mirrored = $$line[9] eq 'Y'?1:0;
      my $v_isnumber = $$line[8] ne '';
      my $v_number = $$line[8];
      $v_number = 0 if ($v_number eq '');
      $res = eval($format);
    };
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
