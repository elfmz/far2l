#include "colorer/viewer/TextLinesStore.h"
#include <cstring>
#include "colorer/Exception.h"
#include "colorer/io/InputSource.h"

TextLinesStore::~TextLinesStore()
{
  freeFile();
}

void TextLinesStore::freeFile()
{
  fileName.reset();
  for (auto it : lines) {
    delete it;
  }
  lines.clear();
}

void TextLinesStore::loadFile(const UnicodeString* inFileName, bool tab2spaces)
{
  if (this->fileName) {
    freeFile();
  }

  uUnicodeString file;

  if (inFileName == nullptr) {
    char line[512];
    size_t read_len;
    std::vector<char> stdin_array;

    while ((read_len = fread(line, 1, sizeof(line), stdin)) != 0) {
      stdin_array.resize(stdin_array.size() + read_len);
      memcpy(&stdin_array[stdin_array.size() - read_len], &line[0], read_len * sizeof(char));
    }
    file = Encodings::toUnicodeString(stdin_array.data(), (int32_t) stdin_array.size());
  }
  else {
    this->fileName = std::make_unique<UnicodeString>(*inFileName);
    colorer::InputSource* is = colorer::InputSource::newInstance(inFileName);

    const byte* data;
    try {
      data = is->openStream();
    } catch (InputSourceException&) {
      delete is;
      throw;
    }
    file = Encodings::toUnicodeString((char*) data, (int32_t) is->length());
    delete is;
  }

  auto length = file->length();
  lines.reserve(static_cast<size_t>(length / 30));  // estimate number of lines

  int filepos = 0;
  int prevpos = 0;

  while (filepos < length + 1) {
    if (filepos == length || (*file)[filepos] == '\r' || (*file)[filepos] == '\n') {
      lines.push_back(new UnicodeString(*file, prevpos, filepos - prevpos));
      if (tab2spaces) {
        replaceTabs(lines.size() - 1);
      }
      if (filepos + 1 < length && (*file)[filepos] == '\r' && (*file)[filepos + 1] == '\n') {
        filepos++;
      }
      prevpos = filepos + 1;
    }
    filepos++;
  }
}

const UnicodeString* TextLinesStore::getFileName() const
{
  return fileName.get();
}

UnicodeString* TextLinesStore::getLine(size_t lno)
{
  if (lines.size() <= lno) {
    return nullptr;
  }
  return lines[lno];
}

size_t TextLinesStore::getLineCount() const
{
  return lines.size();
}

void TextLinesStore::replaceTabs(size_t lno)
{
  lines.at(lno)->findAndReplace("\t", "    ");
}