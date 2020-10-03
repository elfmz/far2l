#ifndef _COLORER_DSTRING_H_
#define _COLORER_DSTRING_H_

#include <colorer/unicode/SString.h>
#include <colorer/unicode/CString.h>

/** Dynamic string class.
    Simple unicode wrapper over any other source.
    @ingroup unicode
*/
class DString : public SString
{
public:
  /** String from any @c String implementing interface.
      @param cstring String class instance, can't be null.
      @param s Starting string position.
      @param l Length of created string. If npos, autodetects string length with
             cstring.length() call.
  */
  DString(const SString* cstring, size_t s = 0, size_t l = npos);

  /** String from any @c String implementing interface.
      @param cstring String class instance.
      @param s Starting string position.
      @param l Length of created string. If npos, autodetects string length with
             cstring.length() call.
  */
  DString(const SString &cstring, size_t s = 0, size_t l = npos);

  ~DString() {};

  DString(DString const &) = delete;
  DString &operator=(DString const &) = delete;
  DString(DString &&);
  DString &operator=(DString &&);

  wchar operator[](size_t i) const override;
  size_t length() const override;

  const SString* str;
  size_t start;
  size_t len;
};

inline DString::DString(const SString* cstring, size_t s, size_t l)
{
  str = cstring;
  start = s;
  len = l;
  if (s > cstring->length() || (len != npos && len > cstring->length() - start))
    throw Exception(CString("bad string constructor parameters"));
  if (len == npos)
    len = cstring->length() - start;
}

inline DString::DString(const SString &cstring, size_t s, size_t l)
{
  str = &cstring;
  start = s;
  len = l;
  if (s > cstring.length() || (len != npos && len > cstring.length() - start))
    throw Exception(CString("bad string constructor parameters"));
  if (len == npos)
    len = cstring.length() - start;
}

inline DString::DString(DString &&cstring):
  str(cstring.str),
  start(cstring.start),
  len(cstring.len)
{
  cstring.str = nullptr;
  cstring.len = 0;
  cstring.start = 0;
}

inline DString &DString::operator=(DString &&cstring)
{
  if (this != &cstring) {
    str = cstring.str;
    len = cstring.len;
    start = cstring.start;

    cstring.str = nullptr;
    cstring.len = 0;
    cstring.start = 0;
  }
  return *this;
}

inline wchar DString::operator[](size_t i) const
{
  return (*str)[start + i];
}

inline size_t DString::length() const
{
  return len;
}

#endif



