#include <cstdio>
#include <colorer/Common.h>
#include <colorer/unicode/Encodings.h>
#include <colorer/io/StreamWriter.h>


StreamWriter::StreamWriter(){}

void StreamWriter::init(FILE *fstream, int encoding, bool useBOM){
  
  if (fstream == nullptr) throw Exception(CString("Invalid stream"));
  file = fstream;
  if (encoding == -1) encoding = Encodings::getDefaultEncodingIndex();
  encodingIndex = encoding;
  this->useBOM = useBOM;
  writeBOM();
}

void StreamWriter::writeBOM(){
  if (useBOM && Encodings::isMultibyteEncoding(encodingIndex)) write(0xFEFF);
}

StreamWriter::StreamWriter(FILE *fstream, int encoding = -1, bool useBOM = true){
  init(fstream, encoding, useBOM);
}

StreamWriter::~StreamWriter(){
}

void StreamWriter::write(wchar c){
  byte buf[8];
  int bufLen = Encodings::toBytes(encodingIndex, c, buf);
  for(int pos = 0; pos < bufLen; pos++)
    putc(buf[pos], file);
}


