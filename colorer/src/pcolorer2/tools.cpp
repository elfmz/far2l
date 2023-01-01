#include "tools.h"
#include <string>
#include <utils.h>

wchar_t *rtrim(wchar_t* str)
{
  wchar_t *ptr = str;
  str += wcslen(str);

  while (iswspace(*(--str))) *str = 0;

  return ptr;
}

wchar_t *ltrim(wchar_t* str)
{
  while (iswspace(*(str++)));

  return str - 1;
}

wchar_t *trim(wchar_t* str)
{
  return ltrim(rtrim(str));
}

/**
  Function converts a path in the UNC path. 
  Source path can be framed by quotes, be a relative, or contain environment variables 
*/
wchar_t *PathToFull(const wchar_t *path, bool unc)
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
  //new_path.resize(p);

  wchar_t *out = new wchar_t[p];
  wmemcpy(out, &new_path[0], p);

  if (unc) {
    fprintf(stderr, "Wanna UNC for '%ls'\n", out);
  }

  return out;
}

SString *PathToFullS(const wchar_t *path, bool unc)
{
  SString *spath=nullptr;
  wchar_t *t=PathToFull(path,unc);
  if (t){
    spath=new StringBuffer(t);
  }
  delete[] t;
  return spath;
}

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
