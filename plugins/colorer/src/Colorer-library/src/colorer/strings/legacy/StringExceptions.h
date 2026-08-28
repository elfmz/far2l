#ifndef COLORER_STRINGEXCEPTIONS_H
#define COLORER_STRINGEXCEPTIONS_H


#include <colorer/Exception.h>

/** Unknown encoding exception.
@ingroup unicode
*/
class UnsupportedEncodingException : public Exception
{
 public:
  explicit UnsupportedEncodingException(const UnicodeString& msg) noexcept
      : Exception("[UnsupportedEncodingException] " + msg)
  {
  }
};

/** Index of requested character is out of bounds.
@ingroup unicode
*/
class StringIndexOutOfBoundsException : public Exception
{
 public:
  explicit StringIndexOutOfBoundsException(const UnicodeString& msg) noexcept
      : Exception("[StringIndexOutOfBoundsException] " + msg)
  {
  }
};

#endif  // COLORER_STRINGEXCEPTIONS_H
