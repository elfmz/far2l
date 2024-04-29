#include "colorer/xml/LocalFileXmlInputSource.h"
#include <filesystem>
#include <memory>
#include <xercesc/util/BinFileInputStream.hpp>
#include "colorer/Exception.h"

LocalFileXmlInputSource::LocalFileXmlInputSource(const XMLCh* path, const XMLCh* base)
{
  auto upath = UnicodeString(path);
  auto ubase = UnicodeString(base);
  auto clear_path = XmlInputSource::getClearFilePath(&ubase, &upath);

  if (std::filesystem::is_regular_file(clear_path)) {
    source_path = std::make_unique<UnicodeString>(clear_path.u16string().c_str());
    input_source = std::make_unique<xercesc::LocalFileInputSource>(clear_path.u16string().c_str());
    // file is not open yet, only after makeStream
  }
  else {
    throw InputSourceException(UnicodeString(clear_path.c_str()) + " isn't regular file.");
  }
}

xercesc::BinInputStream* LocalFileXmlInputSource::makeStream() const
{
  auto stream = std::make_unique<xercesc::BinFileInputStream>(input_source->getSystemId());
  if (!stream->getIsOpen()) {
    throw InputSourceException("Can't open file '" + this->getPath() + "'");
  }
  return stream.release();
}

xercesc::InputSource* LocalFileXmlInputSource::getInputSource() const
{
  return input_source.get();
}
