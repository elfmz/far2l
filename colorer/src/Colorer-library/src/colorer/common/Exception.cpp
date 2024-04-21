#include "colorer/Exception.h"
#include "colorer/Common.h"

Exception::Exception(const char* msg) noexcept : what_str {msg} {}

Exception::Exception(const UnicodeString& msg) noexcept
{
  what_str = UStr::to_stdstr(&msg);
}

const char* Exception::what() const noexcept
{
  return what_str.c_str();
}

InputSourceException::InputSourceException(const UnicodeString& msg) noexcept
    : Exception("[InputSourceException] " + msg)
{
}
