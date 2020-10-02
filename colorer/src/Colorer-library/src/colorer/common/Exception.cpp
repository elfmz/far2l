#include <colorer/Exception.h>

Exception::Exception(const Exception &e) noexcept:
  what_str(e.what_str)
{
}

Exception::Exception() noexcept
{
}

Exception::Exception(const char* msg) noexcept:
  what_str(msg)
{
}

Exception::Exception(const String &msg) noexcept:
  what_str(msg)
{
}

Exception &Exception::operator=(const Exception &e) noexcept
{
  what_str = e.what_str;
  return *this;
}

Exception::~Exception()
{
}

const char* Exception::what() const noexcept
{
  return what_str.getChars();
}

InputSourceException::InputSourceException() noexcept:
  Exception("[InputSourceException] ")
{
}

InputSourceException::InputSourceException(const String &msg) noexcept:
  InputSourceException()
{
  what_str.append(msg);
}


