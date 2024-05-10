#include "tools.h"
#include <utils.h>
#include <string>

wchar_t* rtrim(wchar_t* str)
{
  wchar_t* ptr = str;
  str += wcslen(str);

  while (iswspace(*(--str))) *str = 0;

  return ptr;
}

wchar_t* ltrim(wchar_t* str)
{
  while (iswspace(*(str++)))
    ;

  return str - 1;
}

wchar_t* trim(wchar_t* str)
{
  return ltrim(rtrim(str));
}

/**
  Function converts a path in the UNC path.
  Source path can be framed by quotes, be a relative, or contain environment variables
*/
wchar_t* PathToFull(const wchar_t* path, bool unc)
{
  std::wstring new_path(path);
  if (new_path.empty()) {
    return nullptr;
  }
  // we remove quotes, if they are present, focusing on the first character
  // if he quote it away and the first and last character.
  // If the first character quote, but the latter does not - well, it's not our
  // problem, and so and so error
  if (new_path.size() > 1 && new_path.front() == L'"' && new_path.back() == '"') {
    new_path = new_path.substr(1, new_path.size() - 2);
  }

  // replace the environment variables to their values
  std::string new_path_mb;
  StrWide2MB(new_path, new_path_mb);
  Environment::ExpandString(new_path_mb, false);
  StrMB2Wide(new_path_mb, new_path);

  // take the full path to the file, converting all kinds of ../ ./ etc
  size_t unconverted_len = new_path.size();
  size_t p = FSF.ConvertPath(CPM_FULL, &new_path[0], nullptr, 0);
  if (p > unconverted_len) {
    new_path.resize(p);
    std::fill(new_path.begin() + unconverted_len, new_path.end(), 0);
  }
  p = FSF.ConvertPath(CPM_FULL, &new_path[0], &new_path[0], static_cast<int>(new_path.size()));
  ASSERT(p <= new_path.size());
  // new_path.resize(p);

  wchar_t* out = new wchar_t[p];
  wmemcpy(out, &new_path[0], p);

  if (unc) {
    fprintf(stderr, "Wanna UNC for '%ls'\n", out);
  }

  return out;
}

UnicodeString* PathToFullS(const wchar_t* path, bool unc)
{
  UnicodeString* spath = nullptr;
  wchar_t* t = PathToFull(path, unc);
  if (t) {
    spath = new UnicodeString(t);
  }
  delete[] t;
  return spath;
}

/* ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * ***** END LICENSE BLOCK ***** */
