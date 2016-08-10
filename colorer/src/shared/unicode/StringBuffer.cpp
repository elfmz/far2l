
#include<unicode/StringBuffer.h>

StringBuffer::StringBuffer():SString()
{
  alloc = 20;
  wstr = new wchar[alloc];
}

StringBuffer::StringBuffer(int alloc) : SString()
{
  this->alloc = alloc;
  wstr = new wchar[alloc];
}

StringBuffer::StringBuffer(const char *string, int s, int l)
              :SString(DString(string, s, l)){ alloc = length(); };
StringBuffer::StringBuffer(const String *cstring, int s, int l)
              :SString(cstring, s, l){ alloc = length(); };
StringBuffer::StringBuffer(const String &cstring, int s, int l)
              :SString(cstring, s, l){ alloc = length(); };
StringBuffer::~StringBuffer(){};

void StringBuffer::setLength(int newLength){
  if (newLength > alloc){
    wchar *wstr_new = new wchar[newLength*2];
    alloc = newLength*2;
    for(int i = 0; i < newLength; i++){
      if (i < len) wstr_new[i] = wstr[i];
      else wstr_new[i] = 0;
    };
    delete[] wstr;
    wstr = wstr_new;
  }
  len = newLength;
}

StringBuffer &StringBuffer::append(const String *string){
  if (string == null)
    return append(DString("null"));
  return append(*string);
}

StringBuffer &StringBuffer::append(const String &string){
  int len_new = len+string.length();
  if (alloc > len_new){
    for(int i = len; i < len_new; i++)
      wstr[i] = string[i-len];
  }else{
    wchar *wstr_new = new wchar[len_new*2];
    alloc = len_new*2;
    for(int i = 0; i < len_new; i++){
      if (i < len) wstr_new[i] = wstr[i];
      else wstr_new[i] = string[i-len];
    };
    delete[] wstr;
    wstr = wstr_new;
  };
  len = len_new;
  return *this;
}

StringBuffer &StringBuffer::append(wchar c){
  setLength(len+1);
  wstr[len-1] = c;
  return *this;
}

StringBuffer &StringBuffer::operator+(const String &string){
  return append(string);
}
StringBuffer &StringBuffer::operator+(const String *string){
  return append(string);
}
StringBuffer &StringBuffer::operator+(const char *string){
  return append(DString(string));
}
StringBuffer &StringBuffer::operator+=(const char *string){
  return operator+(DString(string));
}
StringBuffer &StringBuffer::operator+=(const String &string){
  return operator+(string);
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
