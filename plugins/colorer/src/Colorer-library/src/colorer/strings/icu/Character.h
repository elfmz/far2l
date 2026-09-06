#ifndef COLORER_CHARACTER_H
#define COLORER_CHARACTER_H

#include "colorer/strings/icu/common_icu.h"
#include "unicode/uchar.h"

class Character
{
 public:
  static bool isWhitespace(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return c == 0x20 || (c >= 0x09 && c <= 0x0D);
    }
    return u_isspace(c);
  }

  static bool isLowerCase(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return c >= 0x61 && c <= 0x7A;
    }
    return u_islower(c);
  }

  static bool isUpperCase(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return c >= 0x41 && c <= 0x5A;
    }
    return u_isupper(c);
  }

  static bool isLetter(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
    }
    return u_isalpha(c);
  }

  static bool isLetterOrDigitOrUnderscore(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A) || (c >= 0x30 && c <= 0x39) ||
          c == 0x5F;
    }
    return u_isdigit(c) || u_isalpha(c) || c == 0x5F;
  }

  static bool isDigit(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      return c >= 0x30 && c <= 0x39;
    }
    return u_isdigit(c);
  }

  static UChar toLowerCase(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      if (c >= 0x41 && c <= 0x5A) {
        return static_cast<UChar>(c + 0x20);
      }
      return c;
    }
    return static_cast<UChar>(u_tolower(c));
  }

  static UChar toUpperCase(UChar c)
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      if (c >= 0x61 && c <= 0x7A) {
        return static_cast<UChar>(c - 0x20);
      }
      return c;
    }
    return static_cast<UChar>(u_toupper(c));
  }

  static UChar toTitleCase(UChar c);
};

#endif  // COLORER_CHARACTER_H
