#ifndef COLORER_COMMON_ICU_H
#define COLORER_COMMON_ICU_H

#include "unicode/uniset.h"
#include <memory>

using UnicodeString = icu::UnicodeString;
using uUnicodeString = std::unique_ptr<UnicodeString>;
using CharacterClass = icu::UnicodeSet;

constexpr UChar BAD_WCHAR = 0xFFFF;

// system dependent byte
using byte = unsigned char;

#endif  // COLORER_COMMON_ICU_H
