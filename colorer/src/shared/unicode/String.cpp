

#include<unicode/String.h>

#ifdef __unix__
extern "C" int stricmp(const char *c1, const char *c2)
{
  return strcasecmp(c1, c2);
/*
	unsigned int i1, i2;
  while(*c1 || *c2){
    i1 = (unsigned short)Character::toLowerCase(*c1);
    i2 = (unsigned short)Character::toLowerCase(*c2);
    if (i1 < i2) return -1;
    if (i1 > i2) return 1;
    if (!i1) return -1;
    if (!i2) return 1;
    c1++;
    c2++;
  };
  return 0;*/
}

extern "C" int strnicmp(const char *c1, const char *c2, unsigned int len)
{
  return strncasecmp(c1, c2, len);
/*
unsigned int i1, i2;
  while((*c1 || *c2) && len){
    i1 = Character::toLowerCase(*c1);
    i2 = Character::toLowerCase(*c2);
    if (i1 < i2) return -1;
    if (i1 > i2) return 1;
    if (!i1) return -1;
    if (!i2) return 1;
    c1++;
    c2++;
    len--;
  };
  return 0;*/
}
#endif


StringIndexOutOfBoundsException::StringIndexOutOfBoundsException(){};
StringIndexOutOfBoundsException::StringIndexOutOfBoundsException(const String& msg){
  message = new StringBuffer("StringIndexOutOfBoundsException: ");
  message->append(msg);
}



String::String(){
  ret_char_val = null;
  ret_wchar_val = null;
}
String::~String(){
  delete[] ret_char_val;
  delete[] ret_wchar_val;
}

bool String::operator==(const String &str) const{
  if (str.length() != length()) return false;
  for(int i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i]) return false;
  return true;
}

bool String::operator==(const char *str) const{
  return operator==(DString(str));
}

bool String::operator!=(const String &str) const{
  if (str.length() != this->length()) return true;
  for(int i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i]) return true;
  return false;
}

bool String::operator!=(const char *str) const{
  return operator!=(DString(str));
}

bool String::operator>(const String &str) const{
  for(int i = 0; i < str.length() && i < this->length(); i++)
    if ((*this)[i] < str[i] ) return false;
  if (this->length() > str.length()) return true;
  return false;
}

bool String::operator<(const String &str) const{
  for(int i = 0; i < str.length() && i < this->length(); i++){
    if ((*this)[i] > str[i]) return false;
  };
  if (this->length() < str.length()) return true;
  return false;
}


bool String::equals(const String *str) const{
  if (str == null) return false;
  return this->operator==(*str);
}

bool String::equals(const char *str) const{
  return operator==(DString(str));
}

bool String::equalsIgnoreCase(const String *str) const{
  if (!str || str->length() != length()) return false;
  for(int i = 0; i < str->length(); i++)
    if (Character::toLowerCase((*str)[i]) != Character::toLowerCase((*this)[i]) ||
        Character::toUpperCase((*str)[i]) != Character::toUpperCase((*this)[i])) return false;
  return true;
}

int String::compareTo(const String &str) const{
  int i;
  int sl = str.length();
  int l = length();
  for(i = 0; i < sl && i < l; i++){
    int cmp = str[i] - (*this)[i];
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  };
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}

int String::compareToIgnoreCase(const String &str) const{
  int i;
  int sl = str.length();
  int l = length();
  for(i = 0; i < sl && i < l; i++){
    int cmp = Character::toLowerCase(str[i]) - Character::toLowerCase((*this)[i]);
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  };
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}


int String::getWChars(wchar **chars) const{
  *chars = new wchar[length()+1];
  int i;
  for(i = 0; i < length(); i++){
    (*chars)[i] = (*this)[i];
  }
  (*chars)[i] = 0;
  return length();
}

int String::getBytes(byte **bytes, int encoding) const{
  if (encoding == -1) encoding = Encodings::getDefaultEncodingIndex();
  int len = length();
  if (encoding == Encodings::ENC_UTF16 || encoding == Encodings::ENC_UTF16BE) len = len*2;
  if (encoding == Encodings::ENC_UTF32 || encoding == Encodings::ENC_UTF32BE) len = len*4;
  *bytes = new byte[len+1];
  byte buf[8];
  int cpos = 0;
  for(int i = 0; i < length(); i++){
    int retLen = Encodings::toBytes(encoding, (*this)[i], buf);
    // extend byte buffer
    if (cpos+retLen > len){
      if (i == 0) len = 8;
      else len = (len*length())/i + 8;
      byte *copy_buf = new byte[len+1];
      for(int cp = 0; cp < cpos; cp++){
        copy_buf[cp] = (*bytes)[cp];
      };
      delete[] *bytes;
      *bytes = copy_buf;
    };
    for(int cpidx = 0; cpidx < retLen; cpidx++)
      (*bytes)[cpos++] = buf[cpidx];
  };
  (*bytes)[cpos] = 0;
  return cpos;
}

const char *String::getChars(int encoding) const{
  delete [] ret_char_val;
  getBytes((byte**)&ret_char_val, encoding);
  return ret_char_val;
}

const wchar *String::getWChars() const{
  delete [] ret_wchar_val;
  getWChars((wchar**)&ret_wchar_val);
  return ret_wchar_val;
}

int String::indexOf(wchar wc, int pos) const{
  int idx;
  for(idx = pos; idx < this->length() && (*this)[idx] != wc; idx++);
  return idx == this->length()?-1:idx;
}

int String::indexOf(const String &str, int pos) const{
  int thislen = this->length();
  int strlen = str.length();
  for(int idx = pos; idx < thislen - strlen + 1; idx++){
    int idx2;
    for(idx2 = 0; idx2 < strlen; idx2++){
      if (str[idx2] != (*this)[idx+idx2]) break;
    };
    if (idx2 == strlen) return idx;
  };
  return -1;
}

int String::indexOfIgnoreCase(const String &str, int pos) const{
  int thislen = this->length();
  int strlen = str.length();
  for(int idx = pos; idx < thislen - strlen + 1; idx++){
    int idx2;
    for(idx2 = 0; idx2 < strlen; idx2++){
      if (Character::toLowerCase(str[idx2]) != Character::toLowerCase((*this)[idx+idx2])) break;
    };
    if (idx2 == strlen) return idx;
  };
  return -1;
}

int String::lastIndexOf(wchar wc, int pos) const{
  int idx;
  if (pos == -1) pos = this->length();
  if (pos > this->length()) return -1;
  for(idx = pos; idx > 0 && (*this)[idx-1] != wc; idx--);
  return idx == 0?-1:idx-1;
}

int String::lastIndexOf(const String &str, int pos) const{
  if (pos == -1) pos = this->length();
  int str_len = str.length();
  if (pos+str_len > this->length()) return -1;
  for(int idx = pos; idx > 0; idx--){
    int idx2;
    for(idx2 = 0; idx2 != -1 && idx2 < str_len && idx+idx2 < this->length(); idx2++){
      if (str[idx2] != (*this)[idx-1+idx2]) idx2 = -2;
    };
    if (idx2 != -1) return idx-1;
  };
  return -1;
}

bool String::startsWith(const String &str, int pos) const{
  int thislen = this->length();
  int strlen = str.length();
  for(int idx = 0; idx < strlen; idx++){
    if (idx+pos >= thislen) return false;
    if (str[idx] != (*this)[pos+idx]) return false;
  };
  return true;
}

String *String::replace(const String &pattern, const String &newstring) const{
  int copypos = 0;
  int epos = 0;

  StringBuffer *newname = new StringBuffer();
  const String &name = *this;

  while(true){
    epos = name.indexOf(pattern, epos);
    if (epos == -1){
      epos = name.length();
      break;
    };
    newname->append(DString(name, copypos, epos-copypos));
    newname->append(newstring);
    epos = epos + pattern.length();
    copypos = epos;
  };
  if (epos > copypos) newname->append(DString(name, copypos, epos-copypos));
  return newname;
}

int String::hashCode() const{
  int hc = 0;
  int len = length();
  for(int i = 0; i < len; i++)
    hc = 31 * hc + (*this)[i];
  return hc;
}

/*
String String::toLowerCase()
{};
String String::toUpperCase();
*/

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
