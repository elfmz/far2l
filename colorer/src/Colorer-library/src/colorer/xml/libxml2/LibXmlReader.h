#ifndef COLORER_LIBXMLREADER_H
#define COLORER_LIBXMLREADER_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <list>
#include "colorer/xml/XMLNode.h"
#include "colorer/xml/XmlInputSource.h"

class LibXmlReader
{
 public:
  explicit LibXmlReader(const UnicodeString& source_file);
  LibXmlReader(const XmlInputSource& source);

  ~LibXmlReader();

  void parse(std::list<XMLNode>& nodes);

  [[nodiscard]] bool isParsed() const
  {
    return xmldoc != nullptr;
  }

 private:
  xmlDocPtr xmldoc;

  void getAttributes(const xmlNode* node, std::unordered_map<UnicodeString, UnicodeString>& data);
  void getChildren(xmlNode* node, XMLNode& result);
  bool populateNode(xmlNode* node, XMLNode& result);
  UnicodeString getElementText(xmlNode* node);

  static xmlParserInputPtr xmlMyExternalEntityLoader(const char* URL, const char* ID, xmlParserCtxtPtr ctxt);
  static void xml_error_func(void* ctx, const char* msg, ...);
};

#endif  // COLORER_LIBXMLREADER_H
