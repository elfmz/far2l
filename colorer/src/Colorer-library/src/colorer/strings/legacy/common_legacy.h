#ifndef COLORER_COMMON_ICU_H
#define COLORER_COMMON_ICU_H

#include "colorer/strings/legacy/UnicodeString.h"

using uUnicodeString = std::unique_ptr<UnicodeString>;

// system dependent byte
using byte = unsigned char;

#endif  // COLORER_COMMON_ICU_H
