#ifndef COLORER_STREAMWRITER_H
#define COLORER_STREAMWRITER_H

#include "colorer/io/Writer.h"
#include <cstdio>

/** Writes data into operating system output stream.
    @ingroup common_io
*/
class StreamWriter : public Writer
{
 public:
  /** Creates StreamWriter, which uses specified stream, UTF-8 encoding, and byte order mark.
      @param fstream Stream to write data to.
      @param useBOM Does it needed to output first
             Unicode Byte Order Mark character.
  */
  StreamWriter(FILE* fstream, bool _useBOM);
  ~StreamWriter() override = default;
  void write(UChar c) override;

 protected:
  StreamWriter() = default;
  void init(FILE* fstream, bool _useBOM);
  void writeBOM();
  FILE* file = nullptr;

 private:
  bool useBOM = false;
};

#endif // COLORER_STREAMWRITER_H
