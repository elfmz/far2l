#ifndef COLORER_COMMON_ENCODINGS_H
#define COLORER_COMMON_ENCODINGS_H

#include "colorer/strings/icu/common_icu.h"

class Encodings
{
 public:
  static constexpr char ENC_UTF8[] = "UTF-8";

  static uUnicodeString toUnicodeString(char* data, int32_t len);
  static uUnicodeString fromUTF8(char* data, int32_t len);
  static uUnicodeString fromUTF8(unsigned char* data);
  static int toUTF8Bytes(UChar, byte*);
};

#endif  // COLORER_COMMON_ENCODINGS_H
