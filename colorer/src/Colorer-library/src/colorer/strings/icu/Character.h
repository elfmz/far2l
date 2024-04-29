#ifndef COLORER_CHARACTER_H
#define COLORER_CHARACTER_H

#include "colorer/strings/icu/common_icu.h"

class Character
{
 public:
  static bool isWhitespace(UChar c);
  static bool isLowerCase(UChar c);
  static bool isUpperCase(UChar c);
  static bool isLetter(UChar c);
  static bool isLetterOrDigit(UChar c);
  static bool isDigit(UChar c);

  static UChar toLowerCase(UChar c);
  static UChar toUpperCase(UChar c);
  static UChar toTitleCase(UChar c);
};

#endif  // COLORER_CHARACTER_H
