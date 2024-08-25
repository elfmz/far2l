#include "colorer/xml/XmlInputSource.h"

XmlInputSource::XmlInputSource(const UnicodeString& source_path) : XmlInputSource(source_path, nullptr) {}

XmlInputSource::~XmlInputSource()
{
#ifndef COLORER_FEATURE_LIBXML
  //xml_input_source child of xercesc classes and need to free before xercesc
  xml_input_source.reset();
  xercesc::XMLPlatformUtils::Terminate();
#endif
}

XmlInputSource::XmlInputSource(const UnicodeString& source_path, const UnicodeString* source_base)
{
#ifdef COLORER_FEATURE_LIBXML
  xml_input_source = std::make_unique<LibXmlInputSource>(&source_path, source_base);
#else
  xercesc::XMLPlatformUtils::Initialize();
  xml_input_source = XercesXmlInputSource::newInstance(&source_path, source_base);
#endif
}

uXmlInputSource XmlInputSource::createRelative(const UnicodeString& relPath) const
{
  auto str = UnicodeString(xml_input_source->getPath());
  return std::make_unique<XmlInputSource>(relPath, &str);
}

UnicodeString& XmlInputSource::getPath() const
{
  return xml_input_source->getPath();
}

#ifndef COLORER_FEATURE_LIBXML
XercesXmlInputSource* XmlInputSource::getInputSource() const
{
  return xml_input_source.get();
}
#endif

bool XmlInputSource::isFileURI(const UnicodeString& path, const UnicodeString* base)
{
  if (path.startsWith(u"jar") || (base && base->startsWith(u"jar"))) {
    return false;
  }
  return true;
}