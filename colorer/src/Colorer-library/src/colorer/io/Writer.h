#ifndef _COLORER_WRITER_H_
#define _COLORER_WRITER_H_

#include "colorer/Common.h"

/** Abstract character writer class.
    Writes specified character sequences into abstract stream.
    @ingroup common_io
*/
class Writer
{
 public:
  virtual ~Writer() = default;
  /** Writes string */
  virtual void write(const UnicodeString& string);
  /** Writes string */
  virtual void write(const UnicodeString* string);
  /** Writes @c num characters of string, starting at @c from position */
  virtual void write(const UnicodeString& string, int from, int num);
  /** Writes @c num characters of string, starting at @c from position */
  virtual void write(const UnicodeString* string, int from, int num);
  /** Writes single character */
  virtual void write(UChar c) = 0;

  Writer(Writer&&) = delete;
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  Writer& operator=(Writer&&) = delete;

 protected:
  Writer() = default;
};

#endif
