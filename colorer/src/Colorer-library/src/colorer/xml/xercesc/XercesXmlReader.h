#ifndef COLORER_XERCESXML_H
#define COLORER_XERCESXML_H

#include <list>
#include <unordered_map>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "colorer/Common.h"
#include "colorer/xml/XMLNode.h"
#include "colorer/xml/XmlInputSource.h"

class XercesXmlReader
{
 public:

  explicit XercesXmlReader(const xercesc::InputSource* in);
  explicit XercesXmlReader(const XmlInputSource& source);

  void parse(std::list<XMLNode>& nodes);

  ~XercesXmlReader();

  [[nodiscard]] bool isParsed() const
  {
    return !saw_error;
  }

 private:
  bool saw_error = false;
  bool populateNode(const xercesc::DOMNode* node, XMLNode& result);
  void getAttributes(const xercesc::DOMElement* node, std::unordered_map<UnicodeString, UnicodeString>& data);
  void getChildren(const xercesc::DOMNode* node, XMLNode& result);
  const XMLCh* getElementText(const xercesc::DOMElement* blkel) const;

  xercesc::XercesDOMParser* xml_parser;
};
#endif  // COLORER_XERCESXML_H
