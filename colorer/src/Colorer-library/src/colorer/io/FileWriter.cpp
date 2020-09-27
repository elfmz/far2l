
#include<cstdio>
#include<colorer/io/FileWriter.h>

FileWriter::FileWriter(const String *fileName, int encoding, bool useBOM){
  file = fopen(fileName->getChars(), "wb+");
  init(file, encoding, useBOM);
}
FileWriter::FileWriter(const String *fileName){
  file = fopen(fileName->getChars(), "wb+");
  init(file, -1, false);
}
FileWriter::~FileWriter(){
  fclose(file);
}



