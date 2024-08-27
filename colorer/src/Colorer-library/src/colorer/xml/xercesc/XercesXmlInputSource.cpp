#include "colorer/xml/xercesc/XercesXmlInputSource.h"
#include "colorer/Exception.h"
#include "colorer/xml/xercesc/LocalFileXmlInputSource.h"
#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
#include "colorer/xml/xercesc/ZipXmlInputSource.h"
#endif

uXercesXmlInputSource XercesXmlInputSource::newInstance(const UnicodeString* path, const UnicodeString* base)
{
  return newInstance(UStr::to_xmlch(path).get(), UStr::to_xmlch(base).get());
}

uXercesXmlInputSource XercesXmlInputSource::newInstance(const XMLCh* path, const XMLCh* base)
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

bool XercesXmlInputSource::isUriFile(const UnicodeString& path, const UnicodeString* base)
{
  if ((path.startsWith(kJar)) || (base && base->startsWith(kJar))) {
    return false;
  }
  return true;
}

uXercesXmlInputSource XercesXmlInputSource::createRelative(const UnicodeString& relPath)
{
  return newInstance(UStr::to_xmlch(&relPath).get(), this->getInputSource()->getSystemId());
}

UnicodeString& XercesXmlInputSource::getPath() const
{
  return *source_path;
}
