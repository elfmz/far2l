#include "colorer/parsers/FileTypeImpl.h"
#include "colorer/parsers/TextParserImpl.h"

#include <shared_mutex>

TextParser::TextParser() : pimpl(spimpl::make_unique_impl<Impl>()) {}

void TextParser::breakParse()
{
  pimpl->breakParse();
}

void TextParser::clearCache()
{
  pimpl->initCache();
}

int TextParser::parse(int from, int num, TextParseMode mode)
{
  // Shared lock: a concurrent HrcLibrary::loadFileType cannot mutate schemes.
  std::shared_lock<std::shared_mutex> hrc_lock;
  FileType* type = pimpl->currentFileType();
  if (type != nullptr && type->pimpl->library_access != nullptr) {
    hrc_lock = std::shared_lock<std::shared_mutex>(*type->pimpl->library_access);
  }
  return pimpl->parse(from, num, mode);
}

bool TextParser::tryParseLine(int line)
{
  std::shared_lock<std::shared_mutex> hrc_lock;
  FileType* type = pimpl->currentFileType();
  if (type != nullptr && type->pimpl->library_access != nullptr) {
    hrc_lock = std::shared_lock<std::shared_mutex>(*type->pimpl->library_access);
  }
  return pimpl->tryParseLine(line);
}

void TextParser::setFileType(FileType* type)
{
  pimpl->setFileType(type);
}

void TextParser::setLineSource(LineSource* lh)
{
  pimpl->setLineSource(lh);
}

void TextParser::setRegionHandler(RegionHandler* rh)
{
  pimpl->setRegionHandler(rh);
}

void TextParser::setMaxBlockSize(int max_block_size)
{
  pimpl->setMaxBlockSize(max_block_size);
}

void TextParser::setChunkLongLines(bool chunk)
{
  pimpl->setChunkLongLines(chunk);
}

bool TextParser::getChunkLongLines() const
{
  return pimpl->getChunkLongLines();
}
