#ifndef _COLORER_STRING_H_
#define _COLORER_STRING_H_

#include <colorer/unicode/CommonString.h>
#include <vector>

#ifdef __unix__
extern "C" int stricmp(const char*c1, const char*c2);
extern "C" int strnicmp(const char*c1, const char*c2, unsigned int n);
#endif

/** Abstract unicode string class.
    @ingroup unicode
*/
class String {
public:
  String();
  virtual ~String();

  virtual wchar operator[](size_t i) const = 0;
  /** String length in unicode characters */
  virtual size_t length() const = 0;

  /** Checks, if two strings are equals */
  bool operator==(const String &str) const;
  /** Checks, if two strings are not equals */
  bool operator!=(const String &str) const;

  /** Checks, if two strings are equals */
  bool equals(const String *str) const;
  /** Checks, if two strings are equals, ignoring Case Folding */
  bool equalsIgnoreCase(const String *str) const;
  /** Compares two strings.
      @return -1 if this < str;
              0  if this == str;
              1  if this > str;
  */
  int compareTo(const String &str) const;
  /** Compares two strings ignoring case
      @return -1 if this < str;
              0  if this == str;
              1  if this > str;
  */
  int compareToIgnoreCase(const String &str) const;

  /** Returns string content in internally supported unicode character array */
  virtual const w4char *getW4Chars() const;
  virtual const w2char *getW2Chars() const;
#if (__WCHAR_MAX__ > 0xffff)
  inline const wchar *getWChars() const { return (const wchar *)getW4Chars(); } 
#else
  inline const wchar *getWChars() const { return (const wchar *)getW2Chars(); } 
#endif

  /** Returns string content in internally supported character array */
  virtual const char *getChars(int encoding = -1) const;

  /** Searches first index of char @c wc, starting from @c pos */
  virtual size_t indexOf(wchar wc, size_t pos = 0) const;
  /** Searches first index of substring @c str, starting from @c pos */
  virtual size_t indexOf(const String &str, size_t pos = 0) const;
  /** Searches first index of substring @c str, starting from @c pos ignoring character case */
  virtual size_t indexOfIgnoreCase(const String &str, size_t pos = 0) const;
  /** Searches last index of character @c wc, ending with @c pos */
  virtual size_t lastIndexOf(wchar wc, size_t pos = npos) const;

  /** Tests, if string starts with specified @c str substring at position @c pos */
  virtual bool startsWith(const String &str, size_t pos = 0) const;

  /** Internal hashcode of string
  */
  size_t hashCode() const;

  static const size_t npos = size_t(-1);

private:
  mutable void *ret_val = nullptr;
};
#endif



