#include "colorer/io/StreamWriter.h"
#include "colorer/Exception.h"

void StreamWriter::init(FILE* fstream, bool _useBOM)
{
  if (fstream == nullptr)
    throw Exception("Invalid stream");
  file = fstream;
  useBOM = _useBOM;
  writeBOM();
}

void StreamWriter::writeBOM()
{
  if (useBOM) {
    putc(0xEF, file);
    putc(0xBB, file);
    putc(0xBF, file);
  }
}

StreamWriter::StreamWriter(FILE* fstream, bool _useBOM = true)
{
  init(fstream, _useBOM);
}

void StreamWriter::write(UChar c)
{
  byte buf[8];
  int bufLen = Encodings::toUTF8Bytes(c, buf);
  for (int pos = 0; pos < bufLen; pos++) putc(buf[pos], file);
}
