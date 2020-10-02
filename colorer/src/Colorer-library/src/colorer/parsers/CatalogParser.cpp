#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <colorer/parsers/CatalogParser.h>
#include <colorer/xml/XmlInputSource.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <colorer/xml/BaseEntityResolver.h>
#include <colorer/xml/XmlTagDefs.h>

void CatalogParser::parse(const String* path)
{
  logger->debug("begin parse catalog.xml");
  hrc_locations.clear();
  hrd_nodes.clear();

  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  BaseEntityResolver resolver;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setXMLEntityResolver(&resolver);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  uXmlInputSource catalogXIS = XmlInputSource::newInstance(path->getW2Chars(), static_cast<XMLCh*>(nullptr));
  xml_parser.parse(*catalogXIS->getInputSource());
  if (error_handler.getSawErrors()) {
    throw CatalogParserException(CString("Error reading catalog.xml."));
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();

  if (!elem || !xercesc::XMLString::equals(elem->getNodeName(), catTagCatalog)) {
    throw CatalogParserException(SString("Incorrect file structure catalog.xml. Main '<catalog>' block not found at file ") + path);
  }

  parseCatalogBlock(elem);

  logger->debug("end parse catalog.xml");
}

void CatalogParser::parseCatalogBlock(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrcSets)) {
        parseHrcSetsBlock(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrdSets)) {
        parseHrdSetsBlock(subelem);
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseCatalogBlock(static_cast<xercesc::DOMElement*>(node));
    }
  }
}

void CatalogParser::parseHrcSetsBlock(const xercesc::DOMElement* elem)
{
  addHrcSetsLocation(elem);
}

void CatalogParser::addHrcSetsLocation(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagLocation)) {
        auto attr_value = subelem->getAttribute(catLocationAttrLink);
        if (*attr_value != xercesc::chNull) {
          hrc_locations.emplace_back(SString(attr_value));
          logger->debug("add hrc location: '{0}'" , hrc_locations.back().getChars());
        } else {
          logger->warn("found hrc with empty location. skip it location.");
        }
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      addHrcSetsLocation(static_cast<xercesc::DOMElement*>(node));
    }
  }
}

void CatalogParser::parseHrdSetsBlock(const xercesc::DOMElement* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrd)) {
        auto hrd = parseHRDSetsChild(subelem);
        if (hrd) hrd_nodes.push_back(std::move(hrd));
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseHrdSetsBlock(static_cast<xercesc::DOMElement*>(node));
    }
  }
}

std::unique_ptr<HRDNode> CatalogParser::parseHRDSetsChild(const xercesc::DOMElement* elem)
{
  const XMLCh* xhrd_class = elem->getAttribute(catHrdAttrClass);
  const XMLCh* xhrd_name = elem->getAttribute(catHrdAttrName);
  const XMLCh* xhrd_desc = elem->getAttribute(catHrdAttrDescription);

  if (*xhrd_class == xercesc::chNull || *xhrd_name == xercesc::chNull) {
    logger->warn("found HRD with empty class/name. skip this record.");
    return nullptr;
  }

  std::unique_ptr<HRDNode> hrd_node(new HRDNode);
  hrd_node->hrd_class = SString(xhrd_class);
  hrd_node->hrd_name = SString(xhrd_name);
  hrd_node->hrd_description = SString(xhrd_desc);

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      if (xercesc::XMLString::equals(node->getNodeName(), catTagLocation)) {
        auto* subelem = static_cast<xercesc::DOMElement*>(node);
        auto attr_value = subelem->getAttribute(catLocationAttrLink);
        if (*attr_value != xercesc::chNull) {
          hrd_node->hrd_location.emplace_back(SString(CString(attr_value)));
          logger->debug("add hrd location '{0}' for {1}:{2}", hrd_node->hrd_location.back().getChars(), hrd_node->hrd_class.getChars(), hrd_node->hrd_name.getChars());
        } else {
          logger->warn("found hrd with empty location. skip it location.");
        }
      }
    }
  }

  if (!hrd_node->hrd_location.empty()) {
    return hrd_node;
  } else {
    logger->warn("skip HRD {0}:{1} - not found valid location", hrd_node->hrd_class.getChars(), hrd_node->hrd_name.getChars());
    return nullptr;
  }
}
