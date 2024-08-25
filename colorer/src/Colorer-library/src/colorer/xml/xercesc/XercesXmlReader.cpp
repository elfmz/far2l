#include "colorer/xml/xercesc/XercesXmlReader.h"
#include "colorer/xml/xercesc/BaseEntityResolver.h"
#include "colorer/xml/xercesc/XmlParserErrorHandler.h"

XercesXmlReader::XercesXmlReader(const XmlInputSource& source) : XercesXmlReader(source.getInputSource()) {}

XercesXmlReader::XercesXmlReader(const xercesc::InputSource* in)
{
  xercesc::XMLPlatformUtils::Initialize();

  xml_parser = new xercesc::XercesDOMParser();
  XmlParserErrorHandler error_handler;
  BaseEntityResolver resolver;
  xml_parser->setErrorHandler(&error_handler);
  xml_parser->setXMLEntityResolver(&resolver);
  xml_parser->setLoadExternalDTD(false);
  xml_parser->setSkipDTDValidation(true);
  xml_parser->setDisableDefaultEntityResolution(true);

  xml_parser->parse(*in);
  saw_error = error_handler.getSawErrors();
}

XercesXmlReader::~XercesXmlReader()
{
  delete xml_parser;
  xercesc::XMLPlatformUtils::Terminate();
}

void XercesXmlReader::parse(std::list<XMLNode>& nodes)
{
  const xercesc::DOMDocument* doc = xml_parser->getDocument();
  const xercesc::DOMElement* root = doc->getDocumentElement();

  XMLNode result;
  populateNode(root, result);
  nodes.push_back(result);
}

bool XercesXmlReader::populateNode(const xercesc::DOMNode* node, XMLNode& result)
{
  if (node->getNodeType() != xercesc::DOMNode::ELEMENT_NODE) {
    return false;
  }

  // don`t use dynamic_cast, see https://github.com/colorer/Colorer-library/issues/32
  const auto* elem = static_cast<const xercesc::DOMElement*>(node);
  result.name = UnicodeString(elem->getNodeName());
  const auto* t = getElementText(elem);
  if (t != nullptr) {
    result.text = UnicodeString(t);
  }

  getChildren(elem, result);

  getAttributes(elem, result.attributes);

  return true;
}

void XercesXmlReader::getAttributes(const xercesc::DOMElement* node,
                                    std::unordered_map<UnicodeString, UnicodeString>& data)
{
  const auto* attrs = node->getAttributes();
  for (size_t i = 0; i < attrs->getLength(); i++) {
    const auto* attr = attrs->item(i);
    data.insert(std::pair(UnicodeString(attr->getNodeName()), UnicodeString(attr->getNodeValue())));
  }
}

void XercesXmlReader::getChildren(const xercesc::DOMNode* node, XMLNode& result)
{
  for (const auto* elem = node->getFirstChild(); elem != nullptr; elem = elem->getNextSibling()) {
    if (elem->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      getChildren(elem, result);
    }
    else {
      if (XMLNode child; populateNode(elem, child)) {
        result.children.push_back(child);
      }
    }
  }
}

const XMLCh* XercesXmlReader::getElementText(const xercesc::DOMElement* blkel) const
{
  const XMLCh* p = nullptr;
  // возможно текст указан как ...
  for (xercesc::DOMNode* child = blkel->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
    if (child->getNodeType() == xercesc::DOMNode::CDATA_SECTION_NODE) {
      // ... блок CDATA:  например <regexp>![CDATA[ match_regexp ]]</regexp>
      const auto* cdata = static_cast<xercesc::DOMCDATASection*>(child);
      p = cdata->getData();
      break;
    }
    if (child->getNodeType() == xercesc::DOMNode::TEXT_NODE) {
      // ... текстовый блок <regexp> match_regexp </regexp>
      const auto* text = static_cast<xercesc::DOMText*>(child);
      const XMLCh* p1 = text->getData();
      auto* temp_string = xercesc::XMLString::replicate(p1);
      // перед блоком CDATA могут быть пустые строки, учитываем это
      xercesc::XMLString::trim(temp_string);
      if (*temp_string != '\0') {
        p = p1;
        xercesc::XMLString::release(&temp_string);
        break;
      }
      xercesc::XMLString::release(&temp_string);
    }
  }
  return p;
}
