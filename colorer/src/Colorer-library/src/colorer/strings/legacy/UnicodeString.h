#ifndef COLORER_UNICODESTRING_H
#define COLORER_UNICODESTRING_H

#include <colorer/strings/legacy/CommonString.h>
#include <cstdint>
#include <memory>

class CString;

class UnicodeString
{
 public:
  /**
   * Empty string constructor
   */
  UnicodeString() = default;
  UnicodeString(const std::nullptr_t /*text*/){};
  UnicodeString(const char* str);
  UnicodeString(const wchar* str);
  UnicodeString(const w2char* str);
  UnicodeString(const uint16_t* str);
  UnicodeString(const char* string, int32_t l);
  UnicodeString(const wchar* string, int32_t l);
  UnicodeString(const w2char* string, int32_t l);

  UnicodeString(const char* string, int32_t l, int enc);

  /* move constructor */
  UnicodeString(UnicodeString&& cstring) noexcept;
  UnicodeString& operator=(UnicodeString&& cstring) noexcept;

  UnicodeString(const UnicodeString& cstring, int32_t s, int32_t l);
  UnicodeString(const UnicodeString& cstring, int32_t s);

  /* copy constructor */
  UnicodeString& operator=(UnicodeString const& cstring);
  UnicodeString(const UnicodeString& cstring);

  UnicodeString& operator+=(const UnicodeString& string);

  UnicodeString& append(const UnicodeString& string);
  UnicodeString& append(const UnicodeString& string, int32_t start, int32_t maxlen);
  /** Compares two strings.
    @return -1 if this < str;
            0  if this == str;
            1  if this > str;
  */
  int8_t compare(const UnicodeString& str) const;

  ~UnicodeString();

  /**
   * String constructor from integer number
   * TODO нужен только для Exception в Encodings, переделать
   */
  UnicodeString(int no);

  //------------------------------

  UnicodeString(const w4char* str);
  UnicodeString(const w4char* string, int32_t s, int32_t l);

  UnicodeString& append(wchar c);

  /** Checks, if two strings are equals */
  bool operator==(const UnicodeString& str) const;
  /** Checks, if two strings are not equals */
  bool operator!=(const UnicodeString& str) const;

  /** Checks, if two strings are equals */
  bool equals(const UnicodeString* str) const;
  /** Checks, if two strings are equals, ignoring Case Folding */
  bool equalsIgnoreCase(const UnicodeString* str) const;


  /** Compares two strings ignoring case
    @return -1 if this < str;
            0  if this == str;
            1  if this > str;
  */
  int8_t caseCompare(const UnicodeString& str) const;

  bool operator<(const UnicodeString & text) const;

  wchar operator[](int32_t i) const;
  /** String length in unicode characters */
  int32_t length() const;

  UnicodeString& trim();

  /** Searches first index of substring @c str, starting from @c pos */
  int32_t indexOf(const UnicodeString& str, int32_t pos = 0) const;
  /** Searches first index of char @c wc, starting from @c pos */
  int32_t indexOf(wchar wc, int32_t pos = 0) const;
  /** Searches first index of substring @c str, starting from @c pos ignoring character case */
  int32_t indexOfIgnoreCase(const UnicodeString& str, int32_t pos = 0) const;
  int32_t lastIndexOf(wchar wc) const;
  /** Locate in this the last occurrence of the characters in text starting at offset start*/
  int32_t lastIndexOf(wchar wc, int32_t start) const;
  /** Locate in this the last occurrence in the range [start, start + length) of the characters in text, using bitwise comparison.*/
  int32_t lastIndexOf(wchar wc, int32_t start, int32_t length) const;

  /** Tests, if string starts with specified @c str substring at position @c pos */
  bool startsWith(const UnicodeString& str, int32_t pos = 0) const;
  bool endsWith(const UnicodeString& text) const;

  UnicodeString& findAndReplace(const UnicodeString& pattern, const UnicodeString& newstring);
  /** Internal hashcode of string
   */
  size_t hashCode() const;

  /** Changes the length of this String */
  void setLength(int32_t newLength);

  /** Returns string content in internally supported unicode character array */
  const w4char* getW4Chars() const;
  const w2char* getW2Chars() const;
#if (__WCHAR_MAX__ > 0xffff)
  inline const wchar* getWChars() const { return (const wchar*) getW4Chars(); }
#else
  inline const wchar* getWChars() const { return (const wchar*) getW2Chars(); }
#endif

  /** Returns string content in internally supported character array */
  const char* getChars(int encoding = -1) const;

  bool isEmpty() const;
  static const int32_t npos = -1;

 private:
  // сама хранимая строка
  std::unique_ptr<wchar[]> wstr;
  // размер строки
  int32_t len = 0;
  // выделенный буфер
  int32_t alloc = 0;

  mutable void* ret_val = nullptr;

  void construct(const UnicodeString* cstring, int32_t s, int32_t l);
  void construct(const CString* cstring, int32_t s, int32_t l);

};

UnicodeString operator+(const UnicodeString& s1, const UnicodeString& s2);

#endif  // COLORER_UNICODESTRING_H
