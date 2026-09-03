#include "colorer/io/FileWriter.h"

FileWriter::FileWriter(const UnicodeString* fileName, bool useBOM)
{
#ifdef _MSC_VER
  fopen_s(&file, UStr::to_stdstr(fileName).c_str(), "wb+");
#else
  file = fopen(UStr::to_stdstr(fileName).c_str(), "wb+");
#endif
  init(file, useBOM);
}

FileWriter::FileWriter(const UnicodeString* fileName) : FileWriter(fileName, false) {}

FileWriter::~FileWriter()
{
  fclose(file);
}
