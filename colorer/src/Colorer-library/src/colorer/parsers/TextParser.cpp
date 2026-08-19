#include "colorer/parsers/FileTypeImpl.h"
#include "colorer/parsers/TextParserImpl.h"

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
  return pimpl->parse(from, num, mode);
}

bool TextParser::tryParseLine(int line)
{
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
