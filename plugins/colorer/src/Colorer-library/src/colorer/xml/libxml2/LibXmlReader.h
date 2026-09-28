#ifndef COLORER_LIBXMLREADER_H
#define COLORER_LIBXMLREADER_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <unordered_map>
#include "colorer/xml/XMLNode.h"
#include "colorer/xml/XmlInputSource.h"

class LibXmlReader
{
 public:
  explicit LibXmlReader(const XmlInputSource& source);

  ~LibXmlReader();

  void parse(XMLNodeList& nodes);

  [[nodiscard]]
  bool isParsed() const
  {
    return parsed;
  }

 private:
  bool parsed {false};
  XMLNodeList nodes;

  explicit LibXmlReader(const UnicodeString& source_file);
  static void getAttributes(const xmlNode* node, std::unordered_map<UnicodeString, UnicodeString>& data);
  static void getChildren(xmlNode* node, XMLNode& result);
  static bool populateNode(xmlNode* node, XMLNode& result);
  static uUnicodeString getElementText(const xmlNode* node);

  static xmlParserInputPtr xmlMyExternalEntityLoader(const char* URL, const char* ID, xmlParserCtxtPtr ctxt);
  static void xml_error_func(void* ctx, const char* msg, ...);

#ifdef COLORER_FEATURE_ZIPINPUTSOURCE
  static xmlParserInputPtr xmlZipEntityLoader(const PathInJar& paths, xmlParserCtxtPtr ctxt);
#endif
};

#endif  // COLORER_LIBXMLREADER_H
