
#include<colorer/io/Writer.h>

void Writer::write(const String &string){
  write(string, 0, string.length());
}
void Writer::write(const String *string){
  write(*string);
}
void Writer::write(const String &string, int from, int num){
  for(int idx = from; idx < from+num; idx++)
    write(string[idx]);
}
void Writer::write(const String *string, int from, int num){
  write(*string, from, num);
}

