#pragma once

#include <Classes.hpp>

namespace Masks {

class TMask : public TObject
{
public:
  explicit TMask(const UnicodeString & Mask);
  bool Matches(const UnicodeString & Str);

private:
  UnicodeString FMask;
};

int CmpName(const wchar_t *pattern, const wchar_t *str, bool CmpNameSearchMode=false);

} // namespace Masks

