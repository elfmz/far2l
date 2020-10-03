#include <colorer/xml/LocalFileXmlInputSource.h>
#include <memory>
#include <xercesc/util/BinFileInputStream.hpp>
#include <xercesc/util/XMLString.hpp>
#include "XStr.h"

LocalFileXmlInputSource::LocalFileXmlInputSource(const XMLCh* path, const XMLCh* base)
{
  input_source.reset( new xercesc::LocalFileInputSource(base, path));
  if (xercesc::XMLString::findAny(path, kPercent) != nullptr) {
    XMLCh* e_path = ExpandEnvironment(path);
    input_source->setSystemId(e_path);
    delete e_path;
  }
}

LocalFileXmlInputSource::~LocalFileXmlInputSource()
{
}

xercesc::BinInputStream* LocalFileXmlInputSource::makeStream() const
{
  std::unique_ptr<xercesc::BinFileInputStream> stream(new xercesc::BinFileInputStream(input_source->getSystemId()));
  if (!stream->getIsOpen()) {
    throw InputSourceException(SString("Can't open file '") + CString(input_source->getSystemId()) + "'");
  }
  return stream.release();
}

uXmlInputSource LocalFileXmlInputSource::createRelative(const XMLCh* relPath) const
{
  return std::unique_ptr<LocalFileXmlInputSource>(new LocalFileXmlInputSource(relPath, input_source->getSystemId()));
}

xercesc::InputSource* LocalFileXmlInputSource::getInputSource()
{
  return input_source.get();
}


