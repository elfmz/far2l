#ifndef _COLORER_WRITER_H_
#define _COLORER_WRITER_H_

#include<colorer/unicode/String.h>

/** Abstract character writer class.
    Writes specified character sequences into abstract stream.
    @ingroup common_io
*/
class Writer{
public:
  virtual ~Writer(){};
  /** Writes string */
  virtual void write(const String &string);
  /** Writes string */
  virtual void write(const String *string);
  /** Writes @c num characters of string, starting at @c from position */
  virtual void write(const String &string, int from, int num);
  /** Writes @c num characters of string, starting at @c from position */
  virtual void write(const String *string, int from, int num);
  /** Writes single character */
  virtual void write(wchar c) = 0;
protected:
  Writer(){};
};

#endif

