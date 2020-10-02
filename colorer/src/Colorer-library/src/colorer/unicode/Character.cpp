#include <colorer/unicode/Character.h>
#include <colorer/unicode/x_tables.h>
#include <colorer/unicode/x_defines.h>
#include <colorer/unicode/x_charcategory_names.h>

wchar Character::toLowerCase(wchar c)
{
  unsigned long c1 = CHAR_PROP(c);
  if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Ll) return c;
  if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Lt) return c + 1;
  return (wchar)(unsigned short)(c - wchar(c1 >> 16));
}

wchar Character::toUpperCase(wchar c)
{
  unsigned long c1 = CHAR_PROP(c);
  if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Lu) return c;
  if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Lt) return c - 1;
  return (wchar)(unsigned short)(c - wchar(c1 >> 16));
}

wchar Character::toTitleCase(wchar c)
{
  unsigned long c1 = CHAR_PROP(c);
  if (TITLE_CASE(c1)) { // titlecase exists
    if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Lu) return c + 1;
    if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Ll) return c - 1;
    return c;
  } else // has no titlecase form
    if (CHAR_CATEGORY(c1) == CHAR_CATEGORY_Ll)
      return (wchar)(unsigned short)(c - wchar(c1 >> 16));
  return c;
}

bool Character::isLowerCase(wchar c)
{
  return CHAR_CATEGORY(CHAR_PROP(c)) == CHAR_CATEGORY_Ll;
}

bool Character::isUpperCase(wchar c)
{
  return CHAR_CATEGORY(CHAR_PROP(c)) == CHAR_CATEGORY_Lu;
}

bool Character::isTitleCase(wchar c)
{
  return CHAR_CATEGORY(CHAR_PROP(c)) == CHAR_CATEGORY_Lt;
}


bool Character::isLetter(wchar c)
{
  unsigned long c1 = CHAR_CATEGORY(CHAR_PROP(c));
  return ((((1 << CHAR_CATEGORY_Lu) |
            (1 << CHAR_CATEGORY_Ll) |
            (1 << CHAR_CATEGORY_Lt) |
            (1 << CHAR_CATEGORY_Lm) |
            (1 << CHAR_CATEGORY_Lo)
           ) >> c1) & 1) != 0;
}

bool Character::isLetterOrDigit(wchar c)
{
  unsigned long c1 = CHAR_CATEGORY(CHAR_PROP(c));
  return ((((1 << CHAR_CATEGORY_Lu) |
            (1 << CHAR_CATEGORY_Ll) |
            (1 << CHAR_CATEGORY_Lt) |
            (1 << CHAR_CATEGORY_Lm) |
            (1 << CHAR_CATEGORY_Lo) |
            (1 << CHAR_CATEGORY_Nd)
           ) >> c1) & 1) != 0;
}

bool Character::isDigit(wchar c)
{
  return CHAR_CATEGORY(CHAR_PROP(c)) == CHAR_CATEGORY_Nd;
}

bool Character::isAssigned(wchar c)
{
  return CHAR_CATEGORY(CHAR_PROP(c)) != CHAR_CATEGORY_Cn;
}

bool Character::isSpaceChar(wchar c)
{
  return ((((1 << CHAR_CATEGORY_Zs) |
            (1 << CHAR_CATEGORY_Zl)  |
            (1 << CHAR_CATEGORY_Zp)
           ) >> CHAR_CATEGORY(CHAR_PROP(c))) & 1) != 0;
}
bool Character::isWhitespace(wchar c)
{
  return (c == 0x20)
         ||
         ((c <= 0x0020) &&
          (((((1 << 0x0009) |
              (1 << 0x000A) |
              (1 << 0x000C) |
              (1 << 0x000D)) >> c) & 1) != 0))
         ||
         (((((1 << CHAR_CATEGORY_Zs) |
             (1 << CHAR_CATEGORY_Zl) |
             (1 << CHAR_CATEGORY_Zp)
            ) >> CHAR_CATEGORY(CHAR_PROP(c))) & 1) != 0);
}

bool Character::isNumber(wchar c)
{
  return NUMBER(CHAR_PROP(c)) != 0;
}

bool Character::toNumericValue(wchar c, float* f)
{
  unsigned long c1 = CHAR_PROP(c);
  if (!NUMBER(c1)) return false;
  *f = CHAR_PROP2(c);
  return true;
}

ECharCategory Character::getCategory(wchar c)
{
  return ECharCategory(CHAR_CATEGORY(CHAR_PROP(c)));
}

char* Character::getCategoryName(wchar c)
{
  return char_category_names[CHAR_CATEGORY(CHAR_PROP(c))];
}

int Character::getCombiningClass(wchar c)
{
  return COMBINING_CLASS(CHAR_PROP(c));
}

bool Character::isMirrored(wchar c)
{
  return MIRRORED(CHAR_PROP(c)) != 0;
}

int Character::sizeofTables()
{
  return sizeof(arr_idxCharInfo) + sizeof(arr_CharInfo) + sizeof(arr_idxCharInfo2) + sizeof(arr_CharInfo2); //-V119
}


