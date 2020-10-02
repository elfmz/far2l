#ifndef _COLORER_CHARACTER_H_
#define _COLORER_CHARACTER_H_

#include <colorer/unicode/CommonString.h>
#include <colorer/unicode/x_charcategory.h>

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
class Character
{
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
  static bool toNumericValue(wchar c, float* f);

  static char* getCategoryName(wchar c);
  static ECharCategory getCategory(wchar c);

  static int getCombiningClass(wchar c);
  static bool isMirrored(wchar c);

  /** @deprecated For debug purposes only. */
  static int sizeofTables();
};

#endif



