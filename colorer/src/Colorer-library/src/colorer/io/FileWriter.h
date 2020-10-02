#ifndef _COLORER_FILEWRITER_H_
#define _COLORER_FILEWRITER_H_

#include<colorer/io/StreamWriter.h>

/** Writes data stream into a file.
    @ingroup common_io
*/
class FileWriter : public StreamWriter{
public:
  /** @param fileName File name, used to write data to.
  */
  FileWriter(const String *fileName);
  /** @param fileName File name, used to write data to.
      @param encoding Output encoding.
      @param useBOM If true, BOM (Byte Order Mark) is written first.
  */
  FileWriter(const String *fileName, int encoding, bool useBOM);
  ~FileWriter();
protected:
};

#endif

