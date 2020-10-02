#include "registry_wide.h"

DWORD rOpenKey(HKEY hReg, const wchar_t *Name, HKEY &hKey)
{
  DWORD dwDisposition;
  if (WINPORT(RegCreateKeyEx)(hReg, Name, 0, nullptr, 0, KEY_ALL_ACCESS,
                              nullptr, &hKey, &dwDisposition) == ERROR_SUCCESS){
      return dwDisposition;
  }
  return 0;

};

LONG rSetValue(HKEY hReg, const wchar_t *VName, DWORD val)
{
  return WINPORT(RegSetValueEx)(hReg, VName, 0, REG_DWORD, (UCHAR*)(&val), 4);
};

LONG rSetValue(HKEY hReg, const wchar_t *VName, DWORD Type, const void *Data, DWORD Len)
{
  return WINPORT(RegSetValueEx)(hReg, VName, 0, Type, (const BYTE*)Data, Len);
};

wchar_t *rGetValueSz(HKEY hReg, const wchar_t *name, const wchar_t *DefaultValue)
{
  wchar_t *Data;
  DWORD i, Len=0;
  i=WINPORT(RegQueryValueEx)(hReg, name, 0, nullptr, nullptr, &Len);
  if (i==ERROR_SUCCESS){
    int l=Len / sizeof(wchar_t);
    Data=new wchar_t[l];
    i=WINPORT(RegQueryValueEx)(hReg, name, 0, nullptr, (PBYTE)Data, &Len);
    if (i==ERROR_SUCCESS){
      return Data;
    }
    else{
      delete [] Data;
    }
  }
  if (DefaultValue){
    size_t k = wcslen(DefaultValue);
    Data = new wchar_t[k+1];
    wcscpy(Data,DefaultValue);
    return Data;
  }
  return nullptr;
};

DWORD rGetValueDw(HKEY hReg, const wchar_t *name, DWORD DefaultValue)
{
  DWORD data = 0;
  DWORD i = sizeof(DWORD);
  if (WINPORT(RegQueryValueEx)(hReg, name, 0, nullptr, (PBYTE)&data, &i) == ERROR_SUCCESS){
    return data;
  }
  else{
    return DefaultValue;
  }
};

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
