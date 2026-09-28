#include "colorer/strings/icu/Character.h"

UChar Character::toTitleCase(UChar c)
{
  return static_cast<UChar>(u_totitle(c));
}
