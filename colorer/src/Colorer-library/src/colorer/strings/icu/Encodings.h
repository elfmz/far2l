#ifndef COLORER_COMMON_ENCODINGS_H
#define COLORER_COMMON_ENCODINGS_H

#include "colorer/strings/icu/common_icu.h"

constexpr char ENC_UTF8[] = "UTF-8";

class Encodings
{
 public:
  static uUnicodeString toUnicodeString(char* data, int32_t len);
  static int toUTF8Bytes(UChar, byte*);
};

#endif  // COLORER_COMMON_ENCODINGS_H
