#ifndef _COLORER_UNICODETOOLS_H_
#define _COLORER_UNICODETOOLS_H_

#include <colorer/unicode/CString.h>

/** Different Unicode methods and tools.
    @ingroup unicode
*/
class UnicodeTools
{
public:
  /// sometimes need it...
  static bool getNumber(const String* pstr, double* res);
  static bool getNumber(const String* pstr, int* res);
  static int getNumber(const String* pstr);
  static int getHex(wchar c);
  static int getHexNumber(const String* pstr);

  /** For '{name}'  returns 'name'
      Removes brackets and returns new dynamic string,
      based on passed string.
      Returns null if parse error occurs.
      @note Returned dynamic string implies, than
            base passed string remains valid until
            accessing it.
  */
  static CString* getCurlyContent(const String &str, int pos);

  /** \\x{2028} \\x23 \\c  - into wchar
      @param str String to parse Escape sequence.
      @param pos Position, where sequence starts.
      @param retPos Returns here string position after parsed
             character escape.
      @return If bad sequence, returns BAD_WCHAR.
              Else converts character escape and returns it's unicode value.
  */
  static wchar getEscapedChar(const String &str, int pos, int &retPos);
};

#endif



