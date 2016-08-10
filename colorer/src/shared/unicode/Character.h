#ifndef _COLORER_CHARACTER_H_
#define _COLORER_CHARACTER_H_
#include<wchar.h>

//#if   defined _MSC_VER

//typedef unsigned short wchar_t;
//typedef wchar_t wchar;

/// default unicode char definition
/*todo#ifndef WCHAR_MAX
#error wchar misconfig
#elif WCHAR_MAX == 0xFFFFFFFE/2
typedef unsigned short wchar;
#else*/
typedef wchar_t wchar;
//todo #endif

/// wide unicode char definition
typedef unsigned long w4char;

#define BAD_WCHAR ((wchar)0xFFFF)
/*
#elif defined __BORLANDC__
typedef wchar_t wchar;
#elif defined __GNUC__
typedef wchar_t wchar;
#else
typedef unsigned short int wchar;
#endif
*/

#include<common/Common.h>
#include<unicode/Encodings.h>
#include<unicode/x_charcategory.h>

/** Character information class.
    \par Basic features:
    - All Unicode information is generated into tables by scripts
      xcharsets.pl  and  xtables_gen.pl
      They are using UnicodeData.txt file and some codepage files, available
      from <a href="http://www.unicode.org/">http://www.unicode.org/</a>
    - Character class supports most Unicode character properties, except for
      Bidirectional char class, and Decomposition information.
      Most of these methods works like Java Character class methods.

    \par Todo:
    - retrieving of bidirectional class information,
    - retrieving of character decomposition mappings (and normalization
      process information),
    - character 'Digit', 'Decimal Digit' properties. You can retrieve
      only 'Number' value property from digit characters.
    - No explicit surrogate characters support. Surrogate pairs are treated as distinct characters.

    @ingroup unicode
*/
class Character{
public:

  static wchar toLowerCase(wchar c);
  static wchar toUpperCase(wchar c);
  static wchar toTitleCase(wchar c);

  static bool isLowerCase(wchar c);
  static bool isUpperCase(wchar c);
  static bool isTitleCase(wchar c);
  static bool isLetter(wchar c);
  static bool isLetterOrDigit(wchar c);
  static bool isDigit(wchar c);
  static bool isAssigned(wchar c);
  static bool isSpaceChar(wchar c);
  static bool isWhitespace(wchar c);

  static bool isNumber(wchar c);
  static bool toNumericValue(wchar c, float *f);

  static char *getCategoryName(wchar c);
  static ECharCategory getCategory(wchar c);

  static int getCombiningClass(wchar c);
  static bool isMirrored(wchar c);

  /** @deprecated For debug purposes only. */
  static int sizeofTables();
};

#endif

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
