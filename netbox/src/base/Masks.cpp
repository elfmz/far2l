#include <Masks.hpp>

namespace Masks {

static int CmpName_Body(const wchar_t * pattern, const wchar_t * str, bool CmpNameSearchMode)
{
  for (;; ++str)
  {
    wchar_t stringc = Upper(*str);
    wchar_t patternc = Upper(*pattern++);

    switch (patternc)
    {
      case 0:
        return !stringc;

      case L'?':
        if (!stringc)
          return FALSE;
        break;

      case L'*':
        if (!*pattern)
          return TRUE;

        if (*pattern == L'.')
        {
          if (pattern[1] == L'*' && !pattern[2])
            return TRUE;

          if (!wcspbrk(pattern, L"*?["))
          {
            const wchar_t * dot = wcsrchr(str, L'.');

            if (!pattern[1])
              return !dot || !dot[1];

            const wchar_t * patdot = wcschr(pattern + 1, L'.');

            if (patdot && !dot)
              return FALSE;

            if (!patdot && dot )
              return !FarStrCmpI(pattern + 1, dot + 1);
          }
        }

        do
        {
          if (CmpName(pattern, str, CmpNameSearchMode))
            return TRUE;
        }
        while (*str++);

        return FALSE;

      case L'[':
      {
        if (!wcschr(pattern, L']'))
        {
          if (patternc != stringc)
            return FALSE;

          break;
        }

        if (*pattern && *(pattern + 1) == L']')
        {
          if (*pattern != *str)
            return FALSE;

          pattern += 2;
          break;
        }

        int match = 0;
        wchar_t rangec;
        while ((rangec = Upper(*pattern++)) != 0)
        {
          if (rangec == L']')
          {
            if (match)
              break;
            else
              return FALSE;
          }

          if (match)
            continue;

          if (rangec == L'-' && *(pattern - 2) != L'[' && *pattern != L']')
          {
            match = (stringc <= Upper(*pattern) &&
                     Upper(*(pattern - 2)) <= stringc);
            pattern++;
          }
          else
            match = (stringc == rangec);
        }

        if (!rangec)
          return FALSE;
      }
      break;

      default:
        if (patternc != stringc)
        {
          if (patternc == L'.' && !stringc && !CmpNameSearchMode)
            return *pattern != L'.' && CmpName(pattern, str, CmpNameSearchMode);
          else
            return FALSE;
        }
        break;
    }
  }
}

int CmpName(const wchar_t * pattern, const wchar_t * str, bool CmpNameSearchMode)
{
  if (!pattern || !str)
    return FALSE;

  //if (skippath)
  //  str=PointToName(str);

  return CmpName_Body(pattern, str, CmpNameSearchMode);
}

TMask::TMask(const UnicodeString & Mask) :
  FMask(Mask)
{
}

bool TMask::Matches(const UnicodeString & Str)
{
  return CmpName(FMask.c_str(), Str.c_str()) == TRUE;
}

} // namespace Masks

