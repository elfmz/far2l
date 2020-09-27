#ifndef _COLORER_EXCEPTION_H__
#define _COLORER_EXCEPTION_H__

#include <exception>
#include <colorer/unicode/SString.h>

/** Exception class.
    Defines throwable exception.
    @ingroup common
*/
class Exception: public std::exception
{
public:
  /** Default constructor
      Creates exception with empty message
  */
  Exception() noexcept;
  Exception(const char* msg) noexcept;
  /** Creates exception with string message
  */
  Exception(const String &msg) noexcept;
  /** Creates exception with included exception information
  */
  Exception(const Exception &e) noexcept;
  Exception &operator =(const Exception &e) noexcept;
  /** Default destructor
  */
  virtual ~Exception() noexcept;

  /** Returns exception message
  */
  virtual const char* what() const noexcept override;
protected:
  /** Internal message container
  */
  SString what_str;
};

/**
    InputSourceException is thrown, if some IO error occurs.
    @ingroup common
*/
class InputSourceException : public Exception
{
public:
  InputSourceException() noexcept;
  InputSourceException(const String &msg) noexcept;
};

#endif

