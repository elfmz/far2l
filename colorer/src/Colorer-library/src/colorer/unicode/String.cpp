#include <colorer/unicode/String.h>
#include <colorer/unicode/Character.h>
#include <colorer/unicode/Encodings.h>

#ifdef __unix__
extern "C" int stricmp(const char* c1, const char* c2)
{
  unsigned int i1, i2;
  while (*c1 || *c2) {
    i1 = Character::toLowerCase(*c1);
    i2 = Character::toLowerCase(*c2);
    if (i1 < i2) return -1;
    if (i1 > i2) return 1;
    if (!i1) return -1;
    if (!i2) return 1;
    c1++;
    c2++;
  }
  return 0;
}

extern "C" int strnicmp(const char* c1, const char* c2, unsigned int len)
{
  unsigned int i1, i2;
  while ((*c1 || *c2) && len) {
    i1 = Character::toLowerCase(*c1);
    i2 = Character::toLowerCase(*c2);
    if (i1 < i2) return -1;
    if (i1 > i2) return 1;
    if (!i1) return -1;
    if (!i2) return 1;
    c1++;
    c2++;
    len--;
  }
  return 0;
}
#endif

String::String()
{
  ret_char_val = nullptr;
  ret_wchar_val = nullptr;
}

String::~String()
{
  delete[] ret_char_val;
  delete[] ret_wchar_val;
}

bool String::operator==(const String &str) const
{
  if (str.length() != length()) return false;
  for (size_t i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i]) return false;
  return true;
}

bool String::operator!=(const String &str) const
{
  if (str.length() != this->length()) return true;
  for (size_t i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i]) return true;
  return false;
}

bool String::equals(const String* str) const
{
  if (str == nullptr) return false;
  return this->operator==(*str);
}

bool String::equalsIgnoreCase(const String* str) const
{
  if (!str || str->length() != length()) return false;
  for (size_t i = 0; i < str->length(); i++)
    if (Character::toLowerCase((*str)[i]) != Character::toLowerCase((*this)[i]) ||
        Character::toUpperCase((*str)[i]) != Character::toUpperCase((*this)[i])) return false;
  return true;
}

int String::compareTo(const String &str) const
{
  size_t i;
  size_t sl = str.length();
  size_t l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = str[i] - (*this)[i];
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  }
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}

int String::compareToIgnoreCase(const String &str) const
{
  size_t i;
  size_t sl = str.length();
  size_t l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = Character::toLowerCase(str[i]) - Character::toLowerCase((*this)[i]);
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  }
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}

size_t String::getWChars(wchar** chars) const
{
  *chars = new wchar[length() + 1];
  size_t i;
  for (i = 0; i < length(); i++) {
    (*chars)[i] = (*this)[i];
  }
  (*chars)[i] = 0;
  return length();
}

size_t String::getBytes(byte** bytes, int encoding) const
{
  if (encoding == -1) encoding = Encodings::getDefaultEncodingIndex();
  size_t len = length();
  if (encoding == Encodings::ENC_UTF16 || encoding == Encodings::ENC_UTF16BE) len = len * 2;
  if (encoding == Encodings::ENC_UTF32 || encoding == Encodings::ENC_UTF32BE) len = len * 4;
  *bytes = new byte[len + 1];
  byte buf[8];
  size_t cpos = 0;
  for (size_t i = 0; i < length(); i++) {
    size_t retLen = Encodings::toBytes(encoding, (*this)[i], buf);
    // extend byte buffer
    if (cpos + retLen > len) {
      if (i == 0) len = 8;
      else len = (len * length()) / i + 8;
      byte* copy_buf = new byte[len + 1];
      for (size_t cp = 0; cp < cpos; cp++) {
        copy_buf[cp] = (*bytes)[cp];
      }
      delete[] *bytes;
      *bytes = copy_buf;
    }
    for (size_t cpidx = 0; cpidx < retLen; cpidx++)
      (*bytes)[cpos++] = buf[cpidx];
  }
  (*bytes)[cpos] = 0;
  return cpos;
}

const char* String::getChars(int encoding) const
{
  delete[] ret_char_val;
  getBytes((byte**)&ret_char_val, encoding);
  return ret_char_val;
}

const wchar* String::getWChars() const
{
  delete[] ret_wchar_val;
  getWChars((wchar**)&ret_wchar_val);
  return ret_wchar_val;
}

size_t String::indexOf(wchar wc, size_t pos) const
{
  size_t idx;
  for (idx = pos; idx < this->length() && (*this)[idx] != wc; idx++) {}
  return idx == this->length() ? npos : idx;
}

size_t String::indexOf(const String &str, size_t pos) const
{
  size_t thislen = this->length();
  size_t strlen = str.length();
  if (thislen < strlen) return npos;
  for (size_t idx = pos; idx < thislen - strlen + 1; idx++) {
    size_t idx2;
    for (idx2 = 0; idx2 < strlen; idx2++) {
      if (str[idx2] != (*this)[idx + idx2]) break;
    }
    if (idx2 == strlen) return idx;
  }
  return npos;
}

size_t String::indexOfIgnoreCase(const String &str, size_t pos) const
{
  size_t thislen = this->length();
  size_t strlen = str.length();
  if (thislen < strlen) return npos;
  for (size_t idx = pos; idx < thislen - strlen + 1; idx++) {
    size_t idx2;
    for (idx2 = 0; idx2 < strlen; idx2++) {
      if (Character::toLowerCase(str[idx2]) != Character::toLowerCase((*this)[idx + idx2])) break;
    }
    if (idx2 == strlen) return idx;
  }
  return -1;
}

size_t String::lastIndexOf(wchar wc, size_t pos) const
{
  size_t idx;
  if (pos == npos) pos = this->length();
  if (pos > this->length()) return npos;
  for (idx = pos; idx > 0 && (*this)[idx - 1] != wc; idx--) {}
  return idx == 0 ? npos : idx - 1;
}

bool String::startsWith(const String &str, size_t pos) const
{
  size_t thislen = this->length();
  size_t strlen = str.length();
  for (size_t idx = 0; idx < strlen; idx++) {
    if (idx + pos >= thislen) return false;
    if (str[idx] != (*this)[pos + idx]) return false;
  }
  return true;
}

size_t String::hashCode() const
{
  size_t hc = 0;
  size_t len = length();
  for (size_t i = 0; i < len; i++)
    hc = 31 * hc + (*this)[i];
  return hc;
}

