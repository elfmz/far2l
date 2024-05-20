#include "colorer/xml/XmlInputSource.h"
#include "colorer/Exception.h"
#include "colorer/xml/LocalFileXmlInputSource.h"
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
#include "colorer/xml/ZipXmlInputSource.h"
#endif

uXmlInputSource XmlInputSource::newInstance(const UnicodeString* path, const UnicodeString* base)
{
  return newInstance(UStr::to_xmlch(path).get(), UStr::to_xmlch(base).get());
}

uXmlInputSource XmlInputSource::newInstance(const XMLCh* path, const XMLCh* base)
{
  if (!path || (*path == '\0')) {
    throw InputSourceException("XmlInputSource::newInstance: path is empty");
  }
  if (xercesc::XMLString::startsWith(path, kJar) || (base != nullptr && xercesc::XMLString::startsWith(base, kJar))) {
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
    return std::make_unique<ZipXmlInputSource>(path, base);
#else
    throw InputSourceException("ZipXmlInputSource not supported");
#endif
  }
  return std::make_unique<LocalFileXmlInputSource>(path, base);
}

bool XmlInputSource::isUriFile(const UnicodeString& path, const UnicodeString* base)
{
  if ((path.startsWith(kJar)) || (base && base->startsWith(kJar))) {
    return false;
  }
  return true;
}

uXmlInputSource XmlInputSource::createRelative(const XMLCh* relPath)
{
  return newInstance(relPath, this->getInputSource()->getSystemId());
}

UnicodeString& XmlInputSource::getPath() const
{
  return *source_path;
}
