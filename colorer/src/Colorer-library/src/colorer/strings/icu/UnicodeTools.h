#ifndef COLORER_UNICODETOOLS_H
#define COLORER_UNICODETOOLS_H

#include "colorer/strings/icu/common_icu.h"

class UnicodeTools
{
 public:
  static int getHex(UChar c);
  static int getHexNumber(const UnicodeString* pstr);
  static int getNumber(const UnicodeString* pstr);

  /** \\x{2028} \\x23 \\c  - into wchar
  @param str String to parse Escape sequence.
  @param pos Position, where sequence starts.
  @param retPos Returns here string position after parsed
         character escape.
  @return If bad sequence, returns BAD_WCHAR.
          Else converts character escape and returns it's unicode value.
*/
  static UChar getEscapedChar(const UnicodeString& str, int pos, int& retPos);

  /** For '{name}'  returns 'name'
  Removes brackets and returns new dynamic string,
  based on passed string.
  Returns null if parse error occurs.
  @note Returned dynamic string implies, than
        base passed string remains valid until
        accessing it.
*/
  static uUnicodeString getCurlyContent(const UnicodeString& str, int pos);
};

#endif  // COLORER_UNICODETOOLS_H
