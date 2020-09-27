#include <stdio.h>
#include <colorer/unicode/SString.h>
#include <colorer/unicode/CString.h>
#include <colorer/unicode/DString.h>

SString::SString(): wstr(nullptr), len(0), alloc(0)
{
}

void SString::construct(const String* cstring, size_t s, size_t l)
{
  if (s > cstring->length()) throw Exception(CString("bad string constructor parameters"));
  if (l == npos) l = cstring->length() - s;
  wstr.reset(new wchar[l]);
  for (len = 0; len < l; len++)
    wstr[len] = (*cstring)[s + len];
  alloc = len;
}

SString::SString(const String* cstring, size_t s, size_t l)
{
  construct(cstring, s, l);
}

SString::SString(const SString &cstring)
{
  construct(&cstring, 0, npos);
}

SString::SString(const char* string, size_t s, size_t l)
{
  CString ds(string, s, l);
  construct(&ds, 0, ds.length());
}

SString::SString(const w2char* string, size_t s, size_t l)
{
  CString ds(string, s, l);
  construct(&ds, 0, ds.length());
}

SString::SString(const w4char* string, size_t s, size_t l)
{
  CString ds(string, s, l);
  construct(&ds, 0, ds.length());
}

SString::SString(const String &cstring, size_t s, size_t l)
{
  construct(&cstring, s, l);
}

SString::SString(char* str, int enc)
{
  CString ds(str, 0, npos, enc);
  construct(&ds, 0, ds.length());
}

SString::SString(const wchar_t* str)
{
#if (__WCHAR_MAX__ > 0xffff)
  CString ds((const w4char *)str, 0, npos);
#else
  CString ds((const w2char *)str, 0, npos);
#endif
  construct(&ds, 0, ds.length());
}

SString::SString(int no)
{
  char text[40];
  sprintf(text, "%d", no);
  CString dtext = CString(text);
  construct(&dtext, 0, npos);
}

SString::SString(size_t no)
{
  char text[40];
  sprintf(text, "%zd", no); //-V111
  CString dtext = CString(text);
  construct(&dtext, 0, npos);
}

SString::~SString()
{
}

void SString::setLength(size_t newLength)
{
  if (newLength > alloc) {
    std::unique_ptr<wchar[]> wstr_new(new wchar[newLength * 2]);
    alloc = newLength * 2;
    for (size_t i = 0; i < newLength; i++) {
      if (i < len) wstr_new[i] = wstr[i];
      else wstr_new[i] = 0;
    }
    wstr = std::move(wstr_new);
  }
  len = newLength;
}

SString &SString::append(const String* string)
{
  if (string == nullptr)
    return append(CString("null"));
  return append(*string);
}

SString &SString::append(const String &string)
{
  size_t len_new = len + string.length();
  if (alloc > len_new) {
    for (size_t i = len; i < len_new; i++)
      wstr[i] = string[i - len];
  } else {
    std::unique_ptr<wchar[]> wstr_new(new wchar[len_new * 2]);
    alloc = len_new * 2;
    for (size_t i = 0; i < len_new; i++) {
      if (i < len) wstr_new[i] = wstr[i];
      else wstr_new[i] = string[i - len];
    }
    wstr = std::move(wstr_new);
  }
  len = len_new;
  return *this;
}

SString &SString::append(wchar c)
{
  setLength(len + 1);
  wstr[len - 1] = c;
  return *this;
}

SString &SString::operator+(const String &string)
{
  return append(string);
}

SString &SString::operator+(const String* string)
{
  return append(string);
}

SString &SString::operator+(const char* string)
{
  return append(CString(string));
}

SString &SString::operator+=(const char* string)
{
  return operator+(CString(string));
}

SString &SString::operator+=(const String &string)
{
  return operator+(string);
}

SString &SString::operator=(SString const &cstring)
{
  construct(&cstring, 0, npos);
  return *this;
}

SString* SString::replace(const String &pattern, const String &newstring) const
{
  size_t copypos = 0;
  size_t epos = 0;

  SString* newname = new SString();
  const SString &name = *this;

  while (true) {
    epos = name.indexOf(pattern, epos);
    if (epos == npos) {
      epos = name.length();
      break;
    }
    newname->append(CString(name, copypos, epos - copypos));
    newname->append(newstring);
    epos = epos + pattern.length();
    copypos = epos;
  }
  if (epos > copypos) newname->append(CString(name, copypos, epos - copypos));
  return newname;
}

int SString::compareTo(const SString &str) const
{
  size_t i;
  size_t sl = str.length();
  size_t l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = str[i] - this->wstr[i];
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  }
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}

int SString::compareTo(const DString &str) const
{
  size_t i;
  size_t sl = str.length();
  size_t l = length();
  for (i = 0; i < sl && i < l; i++) {
    int cmp = str.str->wstr[str.start + i] - this->wstr[i];
    if (cmp > 0) return -1;
    if (cmp < 0) return 1;
  }
  if (i < sl) return -1;
  if (i < l) return 1;
  return 0;
}



