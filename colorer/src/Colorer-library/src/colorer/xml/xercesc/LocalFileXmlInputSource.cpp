#include "colorer/xml/xercesc/LocalFileXmlInputSource.h"
#include <memory>
#include <xercesc/util/BinFileInputStream.hpp>
#include "colorer/Exception.h"
#include "colorer/utils/Environment.h"

LocalFileXmlInputSource::LocalFileXmlInputSource(const XMLCh* path, const XMLCh* base)
{
  auto upath = UnicodeString(path);
  auto ubase = UnicodeString(base);

  UnicodeString full_path;
  if (colorer::Environment::isRegularFile(&ubase, &upath, full_path)) {
    source_path = std::make_unique<UnicodeString>(full_path);
    setSystemId(UStr::to_xmlch(&full_path).get());
    // file is not open yet, only after makeStream
  }
  else {
    throw InputSourceException(full_path + " isn't regular file.");
  }
}

xercesc::BinInputStream* LocalFileXmlInputSource::makeStream() const
{
  auto stream = std::make_unique<xercesc::BinFileInputStream>(UStr::to_xmlch(source_path.get()).get());
  if (!stream->getIsOpen()) {
    throw InputSourceException("Can't open file '" + this->getPath() + "'");
  }
  return stream.release();
}

xercesc::InputSource* LocalFileXmlInputSource::getInputSource() const
{
  return (xercesc::InputSource*) this;
}
