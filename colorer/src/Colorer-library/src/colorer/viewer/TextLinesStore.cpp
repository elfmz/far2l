#include <stdio.h>
#include <string.h>
#include <colorer/viewer/TextLinesStore.h>
#include <colorer/io/InputSource.h>
#include <colorer/unicode/Encodings.h>

void TextLinesStore::replaceTabs(size_t lno)
{
  SString* od = lines.at(lno)->replace(CString("\t"), CString("    "));
  delete lines.at(lno);
  lines.at(lno) = od;
}

TextLinesStore::TextLinesStore()
{
  fileName = nullptr;
}

TextLinesStore::~TextLinesStore()
{
  freeFile();
}

void TextLinesStore::freeFile()
{
  delete fileName;
  fileName = nullptr;
  for(auto it:lines){
    delete it;
  }
  lines.clear();
}

void TextLinesStore::loadFile(const String* fileName_, const String* inputEncoding, bool tab2spaces)
{
  if (this->fileName != nullptr) {
    freeFile();
  }

  if (fileName_ == nullptr) {
    char line[256];
    while (fgets(line, sizeof(line), stdin) != nullptr) {
      strtok(line, "\r\n");
      lines.push_back(new SString(line));
      if (tab2spaces) {
        replaceTabs(lines.size() - 1);
      }
    }
  } else {
    this->fileName = new SString(fileName_);
    colorer::InputSource* is = colorer::InputSource::newInstance(fileName_);

    const byte* data;
    try {
      data = is->openStream();
    } catch (InputSourceException&) {
      delete is;
      throw;
    }
    int len = is->length();

    int ei = inputEncoding == nullptr ? -1 : Encodings::getEncodingIndex(inputEncoding->getChars());
    CString file(data, len, ei);
    int length = file.length();
    lines.reserve(static_cast<size_t>(length / 30)); // estimate number of lines

    int i = 0;
    int filepos = 0;
    int prevpos = 0;
    if (length && file[0] == 0xFEFF) {
      filepos = prevpos = 1;
    }
    while (filepos < length + 1) {
      if (filepos == length || file[filepos] == '\r' || file[filepos] == '\n') {
        lines.push_back(new SString(&file, prevpos, filepos - prevpos));
        if (tab2spaces) {
          replaceTabs(lines.size() - 1);
        }
        if (filepos + 1 < length && file[filepos] == '\r' && file[filepos + 1] == '\n') {
          filepos++;
        } else if (filepos + 1 < length && file[filepos] == '\n' && file[filepos + 1] == '\r') {
          filepos++;
        }
        prevpos = filepos + 1;
        i++;
      }
      filepos++;
    }
    delete is;
  }
}

const String* TextLinesStore::getFileName()
{
  return fileName;
}

SString* TextLinesStore::getLine(size_t lno)
{
  if (lines.size() <= lno) {
    return nullptr;
  }
  return lines[lno];
}

size_t TextLinesStore::getLineCount()
{
  return lines.size();
}


