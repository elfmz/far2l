#include <Classes.hpp>
#include <StrUtils.hpp>
#include <Sysutils.hpp>

UnicodeString ReplaceStr(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  return ::StringReplaceAll(Str, What, ByWhat);
}

bool StartsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return Str.Pos(SubStr) == 1;
}

bool EndsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  if (SubStr.Length() > Str.Length())
    return false;
  return Str.SubStr(Str.Length() - SubStr.Length() + 1, SubStr.Length()) == SubStr;
}
