#include <colorer/unicode/SString.h>
#include "StringBuffer.h"

StringBuffer::StringBuffer():
  SString()
{
  setLength(0);
}

StringBuffer::StringBuffer(int _alloc):
  SString()
{
  setLength(_alloc);
}

StringBuffer::StringBuffer(const char *string, int s, int l):
  SString(string, s, l)
{
}

StringBuffer::StringBuffer(const w2char *string, int s, int l):
  SString(string, s, l)
{
}

StringBuffer::StringBuffer(const w4char *string, int s, int l):
  SString(string, s, l)
{
}

StringBuffer::StringBuffer(const String *cstring, int s, int l):
  SString(cstring, s, l)
{
}

StringBuffer::StringBuffer(const String &cstring, int s, int l):
  SString(cstring, s, l)
{
}

StringBuffer::StringBuffer(const wchar_t *wstring, int s, int l):
  SString()
{
  int i, newLength = 0;
  if (l == -1){
      for (i = s; wstring[i]; i++) newLength++;
    }
    else newLength = l - s;
  if (newLength){
    std::unique_ptr<wchar[]> wstr_new(new wchar[newLength * 2]);
    alloc = newLength * 2;
    for (i = s; i < newLength; i++)
      wstr_new[i - s] = wstring[i] & 0xFFFF;
    wstr.swap(wstr_new);
    len = newLength;
  }
}

StringBuffer::~StringBuffer(){
}

StringBuffer &StringBuffer::append(const String &string)
{
  SString::append(string);
  return *this;
}

StringBuffer &StringBuffer::append(const String* string)
{
  SString::append(string);
  return *this;
}

StringBuffer &StringBuffer::append(wchar c)
{
  SString::append(c);
  return *this;
}

StringBuffer &StringBuffer::append(const wchar_t* wstring)
{
  int i, newLength = 0;
  for (i = 0; wstring[i]; i++) newLength++;
  if (newLength){
    std::unique_ptr<wchar[]> wstr_new(new wchar[(newLength + len) * 2]);
    alloc = (newLength + len) * 2;
    for (size_t i = 0; i < newLength + len; i++){
      wstr_new[i] = (i < len) ? wstr[i] : wstring[i - len] & 0xFFFF;
    }
    wstr.swap(wstr_new);
    len += newLength;
  }
  return *this;
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
