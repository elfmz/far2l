#ifndef _COLORER_STREAMWRITER_H_
#define _COLORER_STREAMWRITER_H_

#include<stdio.h>
#include<colorer/io/Writer.h>

/** Writes data into operating system output stream.
    @ingroup common_io
*/
class StreamWriter : public Writer{
public:
  /** Creates StreamWriter, which uses specified stream,
      encoding, and byte order mark.
      @param fstream Stream to write data to.
      @param encoding output encoding.
      @param useBOM Does it needed to output first
             Unicode Byte Order Mark character.
  */
  StreamWriter(FILE *fstream, int encoding, bool useBOM);
  ~StreamWriter();
  void write(wchar c);
protected:
  StreamWriter();
  void init(FILE *fstream, int encoding, bool useBOM);
  void writeBOM();
  FILE *file;
private:
  int encodingIndex;
  bool useBOM;
};

#endif

