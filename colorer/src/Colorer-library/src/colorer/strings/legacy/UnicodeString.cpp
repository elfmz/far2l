#include <colorer/strings/legacy/CString.h>
#include <colorer/strings/legacy/Character.h>
#include <colorer/strings/legacy/Encodings.h>
#include <colorer/strings/legacy/UnicodeString.h>
#include <cstdlib>
#include "colorer/Exception.h"

UnicodeString operator+(const UnicodeString& s1, const UnicodeString& s2)
{
  return UnicodeString().append(s1).append(s2);
}

UnicodeString::~UnicodeString()
{
  free(ret_val);
}

void UnicodeString::construct(const CString* cstring, int32_t s, int32_t l)
{
  if (s > cstring->length())
    throw Exception("bad string constructor parameters");
  if (l == npos)
    l = cstring->length() - s;
  wstr.reset(new wchar[l]);
  for (len = 0; len < l; len++) wstr[len] = (*cstring)[s + len];
  alloc = len;
}

UnicodeString::UnicodeString(const char* str) : UnicodeString(str, npos) {}

UnicodeString::UnicodeString(const wchar* str) : UnicodeString(str, npos) {}

UnicodeString::UnicodeString(const w2char* str) : UnicodeString(str, npos) {}

UnicodeString::UnicodeString(const w4char* str) : UnicodeString(str, 0, npos) {}

UnicodeString::UnicodeString(const uint16_t* str) : UnicodeString((const w2char*) str, npos) {}

UnicodeString::UnicodeString(const char* string, int32_t l)
{
  if (!string)
    return;
  CString ds(string, 0, l);
  construct(&ds, 0, ds.length());
}

UnicodeString::UnicodeString(const wchar* string, int32_t l)
{
  if (!string)
    return;
#if (__WCHAR_MAX__ > 0xffff)
  CString ds((const w4char*) string, 0, l);
#else
  CString ds((const w2char*) string, 0, l);
#endif
  construct(&ds, 0, ds.length());
}

UnicodeString::UnicodeString(const w2char* string, int32_t l)
{
  if (!string)
    return;
  CString ds(string, 0, l);
  construct(&ds, 0, ds.length());
}

UnicodeString::UnicodeString(const w4char* string, int32_t s, int32_t l)
{
  if (!string)
    return;
  CString ds(string, s, l);
  construct(&ds, 0, ds.length());
}

void UnicodeString::construct(const UnicodeString* cstring, int32_t s, int32_t l)
{
  if (s > cstring->length())
    throw Exception("bad string constructor parameters");
  if (l == npos)
    l = cstring->length() - s;
  wstr.reset(new wchar[l]);
  for (len = 0; len < l; len++) wstr[len] = (*cstring)[s + len];
  alloc = len;
}

UnicodeString::UnicodeString(const UnicodeString& cstring, int32_t s, int32_t l)
{
  construct(&cstring, s, l);
}

UnicodeString::UnicodeString(int no)
{
  CString dtext = CString(std::to_string(no).c_str());
  construct(&dtext, 0, npos);
}

int32_t UnicodeString::length() const
{
  return len;
}

UnicodeString& UnicodeString::trim()
{
  int32_t left;
  for (left = 0; left < len && Character::isWhitespace(wstr[left]); left++) {
  }

  int32_t right;
  for (right = len - 1; right >= 0 && Character::isWhitespace(wstr[right]); right--) {
  }

  int32_t newLength = left < right ? right - left + 1 : 0;
  auto wstr_new = std::make_unique<wchar[]>(newLength);
  for (auto i = 0; i < newLength; i++) {
    wstr_new[i] = wstr[left + i];
  }
  wstr = std::move(wstr_new);
  len = newLength;
  alloc = newLength;
  return *this;
}

wchar UnicodeString::operator[](int32_t i) const
{
  return wstr[i];
}

UnicodeString::UnicodeString(UnicodeString&& cstring) noexcept
    : wstr(std::move(cstring.wstr)), len(cstring.len), alloc(cstring.alloc)
{
  cstring.wstr = nullptr;
  cstring.alloc = 0;
  cstring.len = 0;
}

UnicodeString& UnicodeString::operator=(UnicodeString&& cstring) noexcept
{
  if (this != &cstring) {
    wstr = std::move(cstring.wstr);
    alloc = cstring.alloc;
    len = cstring.len;
    cstring.wstr = nullptr;
    cstring.alloc = 0;
    cstring.len = 0;
  }
  return *this;
}

void UnicodeString::setLength(int32_t newLength)
{
  if (newLength > alloc) {
    auto wstr_new = std::make_unique<wchar[]>(newLength * 2);
    alloc = newLength * 2;
    for (int32_t i = 0; i < newLength; i++) {
      if (i < len)
        wstr_new[i] = wstr[i];
      else
        wstr_new[i] = 0;
    }
    wstr = std::move(wstr_new);
  }
  len = newLength;
}

UnicodeString& UnicodeString::append(const UnicodeString& string)
{
  return append(string, 0, INT32_MAX);
}

UnicodeString& UnicodeString::append(const UnicodeString& string, int32_t start, int32_t maxlen)
{
  const int32_t len_new = len + (maxlen <= string.length() ? maxlen : string.length());

  if (alloc < len_new) {
    auto wstr_new = std::make_unique<wchar[]>(len_new * 2);
    alloc = len_new * 2;
    for (int32_t i = 0; i < len; i++) {
      wstr_new[i] = wstr[i];
    }
    wstr = std::move(wstr_new);
  }

  for (auto i = len; i < len_new; i++) {
    wstr[i] = string[start + i - len];
  }

  len = len_new;
  return *this;
}

UnicodeString& UnicodeString::append(wchar c)
{
  setLength(len + 1);
  wstr[len - 1] = c;
  return *this;
}

UnicodeString& UnicodeString::operator+=(const UnicodeString& string)
{
  return append(string);
}

UnicodeString& UnicodeString::operator=(UnicodeString const& cstring)
{
  if (this == &cstring) {
    return *this;
  }
  construct(&cstring, 0, npos);
  return *this;
}

int8_t UnicodeString::compare(const UnicodeString& str) const
{
  int32_t i;
  auto sl = str.length();
  auto l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = str[i] - this->wstr[i];
    if (cmp > 0)
      return -1;
    if (cmp < 0)
      return 1;
  }
  if (i < sl)
    return -1;
  if (i < l)
    return 1;
  return 0;
}

bool UnicodeString::operator==(const UnicodeString& str) const
{
  if (str.length() != length())
    return false;
  for (auto i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i])
      return false;
  return true;
}

bool UnicodeString::operator!=(const UnicodeString& str) const
{
  if (str.length() != this->length())
    return true;
  for (auto i = 0; i < str.length(); i++)
    if (str[i] != (*this)[i])
      return true;
  return false;
}

bool UnicodeString::equals(const UnicodeString* str) const
{
  if (str == nullptr)
    return false;
  return this->operator==(*str);
}

bool UnicodeString::equalsIgnoreCase(const UnicodeString* str) const
{
  if (!str || str->length() != length())
    return false;
  for (auto i = 0; i < str->length(); i++)
    if (Character::toLowerCase((*str)[i]) != Character::toLowerCase((*this)[i]) ||
        Character::toUpperCase((*str)[i]) != Character::toUpperCase((*this)[i]))
      return false;
  return true;
}

int8_t UnicodeString::caseCompare(const UnicodeString& str) const
{
  int32_t i;
  auto sl = str.length();
  auto l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = Character::toLowerCase(str[i]) - Character::toLowerCase((*this)[i]);
    if (cmp > 0)
      return -1;
    if (cmp < 0)
      return 1;
  }
  if (i < sl)
    return -1;
  if (i < l)
    return 1;
  return 0;
}

int32_t UnicodeString::indexOf(const UnicodeString& str, int32_t pos) const
{
  auto thislen = this->length();
  auto strlen = str.length();
  if (thislen < strlen)
    return npos;
  for (auto idx = pos; idx < thislen - strlen + 1; idx++) {
    int32_t idx2;
    for (idx2 = 0; idx2 < strlen; idx2++) {
      if (str[idx2] != (*this)[idx + idx2])
        break;
    }
    if (idx2 == strlen)
      return idx;
  }
  return npos;
}

int32_t UnicodeString::indexOf(wchar wc, int32_t pos) const
{
  int32_t idx;
  for (idx = pos; idx < this->length() && (*this)[idx] != wc; idx++) {
  }
  return idx == this->length() ? npos : idx;
}

int32_t UnicodeString::indexOfIgnoreCase(const UnicodeString& str, int32_t pos) const
{
  auto thislen = this->length();
  auto strlen = str.length();
  if (thislen < strlen)
    return npos;
  for (auto idx = pos; idx < thislen - strlen + 1; idx++) {
    int32_t idx2;
    for (idx2 = 0; idx2 < strlen; idx2++) {
      if (Character::toLowerCase(str[idx2]) != Character::toLowerCase((*this)[idx + idx2]))
        break;
    }
    if (idx2 == strlen)
      return idx;
  }
  return -1;
}

int32_t UnicodeString::lastIndexOf(wchar wc, int32_t pos) const
{
  int32_t idx;
  if (pos == npos)
    pos = this->length();
  if (pos > this->length())
    return npos;
  for (idx = pos; idx > 0 && (*this)[idx - 1] != wc; idx--) {
  }
  return idx == 0 ? npos : idx - 1;
}

bool UnicodeString::startsWith(const UnicodeString& str, int32_t pos) const
{
  auto thislen = this->length();
  auto strlen = str.length();
  for (auto idx = 0; idx < strlen; idx++) {
    if (idx + pos >= thislen)
      return false;
    if (str[idx] != (*this)[pos + idx])
      return false;
  }
  return true;
}

size_t UnicodeString::hashCode() const
{
  size_t hc = 0;
  for (auto i = 0; i < len; i++) hc = 31 * hc + (*this)[i];
  return hc;
}

const w4char* UnicodeString::getW4Chars() const
{
  // TODO: use real UCS16->UTF32 conversion if needed
  auto ret_w4char_val = (w4char*) realloc(ret_val, (length() + 1) * sizeof(w4char));
  if (!ret_w4char_val)
    return nullptr;
  ret_val = ret_w4char_val;

  int32_t i;
  for (i = 0; i < length(); i++) {
    ret_w4char_val[i] = w4char((*this)[i]);
  }
  ret_w4char_val[i] = 0;
  return ret_w4char_val;
}

const w2char* UnicodeString::getW2Chars() const
{
  // TODO: use real UCS32->UTF16 conversion if needed
  auto ret_w2char_val = (w2char*) realloc(ret_val, (length() + 1) * sizeof(w2char));
  if (!ret_w2char_val)
    return nullptr;
  ret_val = ret_w2char_val;

  int32_t i;
  for (i = 0; i < length(); i++) {
    ret_w2char_val[i] = w2char((*this)[i]);
  }
  ret_w2char_val[i] = 0;
  return ret_w2char_val;
}

const char* UnicodeString::getChars(int encoding) const
{
  if (encoding == -1)
    encoding = Encodings::getDefaultEncodingIndex();
  auto local_len = length();
  if (encoding == Encodings::ENC_UTF16 || encoding == Encodings::ENC_UTF16BE)
    local_len = local_len * 2;
  if (encoding == Encodings::ENC_UTF32 || encoding == Encodings::ENC_UTF32BE)
    local_len = local_len * 4;
  char* ret_char_val = (char*) realloc(ret_val, local_len + 1);
  if (!ret_char_val) {
    return "[NO MEMORY]";
  }
  ret_val = ret_char_val;
  byte buf[8];
  int32_t cpos = 0;
  for (auto i = 0; i < length(); i++) {
    auto retLen = Encodings::toBytes(encoding, (*this)[i], buf);
    // extend byte buffer
    if (cpos + retLen > local_len) {
      if (i == 0)
        local_len = 8;
      else
        local_len = (local_len * length()) / i + 8;
      ret_char_val = (char*) realloc(ret_char_val, local_len + 1);
      if (!ret_char_val) {
        return "[NO MEMORY]";
      }
      ret_val = ret_char_val;
    }
    for (int32_t cpidx = 0; cpidx < retLen; cpidx++) ret_char_val[cpos++] = buf[cpidx];
  }
  ret_char_val[cpos] = 0;

  return ret_char_val;
}

UnicodeString& UnicodeString::findAndReplace(const UnicodeString& pattern, const UnicodeString& newstring)
{
  int32_t copypos = 0;
  int32_t epos = 0;

  auto newname = std::make_unique<UnicodeString>();
  const UnicodeString& name = *this;

  while (true) {
    epos = name.indexOf(pattern, epos);
    if (epos == npos) {
      epos = name.length();
      break;
    }
    newname->append(UnicodeString(name, copypos, epos - copypos));
    newname->append(newstring);
    epos = epos + pattern.length();
    copypos = epos;
  }
  if (epos > copypos)
    newname->append(UnicodeString(name, copypos, epos - copypos));

  *this = *newname;
  return *this;
}

bool UnicodeString::isEmpty() const
{
  return len == 0;
}

UnicodeString::UnicodeString(const UnicodeString& cstring)
{
  construct(&cstring, 0, npos);
}

UnicodeString::UnicodeString(const UnicodeString& cstring, int32_t s) : UnicodeString(cstring, s, npos) {}

UnicodeString::UnicodeString(const char* string, int32_t l, int enc)
{
  if (!string)
    return;
  CString ds((byte*) string, l, enc);
  construct(&ds, 0, ds.length());
}

bool UnicodeString::operator<(const UnicodeString& text) const
{
  return compare(text) == -1;
}
