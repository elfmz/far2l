#include "tools.h"

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
  size_t len=wcslen(path);
  if (!len){
    return nullptr;
  }

  wchar_t *new_path = nullptr;
  // we remove quotes, if they are present, focusing on the first character
  // if he quote it away and the first and last character.
  // If the first character quote, but the latter does not - well, it's not our 
  // problem, and so and so error
  if (*path==L'"'){
    len--;
    new_path=new wchar_t[len];
    wcsncpy(new_path, &path[1],len-1);
    new_path[len-1]='\0';
  }
  else{
    len++;
    new_path=new wchar_t[len];
    wcscpy(new_path, path);
  }

  // replace the environment variables to their values
  size_t i=WINPORT(ExpandEnvironmentStrings)(new_path, nullptr, 0);
  if (i>len){
    len = i;
  }
  wchar_t *temp = new wchar_t[len];
  WINPORT(ExpandEnvironmentStrings)(new_path,temp,static_cast<DWORD>(i));
  delete[] new_path;
  new_path = temp;

  // take the full path to the file, converting all kinds of ../ ./ etc
  size_t p=FSF.ConvertPath(CPM_FULL, new_path, nullptr, 0);
  if (p>len){
    len = p;
    wchar_t *temp = new wchar_t[len];
    wcscpy(temp,new_path);
    delete[] new_path;
    new_path = temp;
  }
  FSF.ConvertPath(CPM_FULL, new_path, new_path, static_cast<int>(len));

  if (unc){
    fprintf(stderr, "Wanna UNC for '%ls'\n",new_path);
  }

  // reduce the length of the buffer
  i = wcslen(new_path)+1;
  if (i!=len){
    wchar_t *temp = new wchar_t[i];
    wcscpy(temp,new_path);
    delete[] new_path;
    return temp;
  }

  return new_path;
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
