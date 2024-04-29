#include "colorer/strings/icu/Character.h"

bool Character::isWhitespace(UChar c)
{
  return u_isspace(c);
}

bool Character::isLowerCase(UChar c)
{
  return u_islower(c);
}

bool Character::isUpperCase(UChar c)
{
  return u_isupper(c);
}

bool Character::isLetter(UChar c)
{
  return u_isalpha(c);
}

bool Character::isLetterOrDigit(UChar c)
{
  return u_isdigit(c) || u_isalpha(c);
}

bool Character::isDigit(UChar c)
{
  return u_isdigit(c);
}

UChar Character::toLowerCase(UChar c)
{
  return (UChar) u_tolower(c);
}

UChar Character::toUpperCase(UChar c)
{
  return (UChar) u_toupper(c);
}

UChar Character::toTitleCase(UChar c)
{
  return (UChar) u_totitle(c);
}
