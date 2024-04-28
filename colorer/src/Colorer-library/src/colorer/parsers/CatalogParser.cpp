#include "colorer/parsers/CatalogParser.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/BaseEntityResolver.h"
#include "colorer/xml/XmlInputSource.h"
#include "colorer/xml/XmlParserErrorHandler.h"

void CatalogParser::parse(const UnicodeString* path)
{
  logger->debug("start parse {0} as catalog.xml", *path);
  hrc_locations.clear();
  hrd_nodes.clear();

  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  BaseEntityResolver resolver;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setXMLEntityResolver(&resolver);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.setDisableDefaultEntityResolution(true);

  uXmlInputSource catalogXIS = XmlInputSource::newInstance(path);
  xml_parser.parse(*catalogXIS->getInputSource());
  if (error_handler.getSawErrors()) {
    throw CatalogParserException(*path + UnicodeString(" parse error"));
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();

  if (!elem || !xercesc::XMLString::equals(elem->getNodeName(), catTagCatalog)) {
    throw CatalogParserException(
        "Incorrect file structure catalog.xml. Main '<catalog>' block not found at file " + *path);
  }

  parseCatalogBlock(elem);

  logger->debug("end parse catalog.xml");
}

void CatalogParser::parseCatalogBlock(const xercesc::DOMNode* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      // don`t use dynamic_cast, see https://github.com/colorer/Colorer-library/issues/32
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrcSets)) {
        parseHrcSetsBlock(subelem);
        continue;
      }
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrdSets)) {
        parseHrdSetsBlock(node);
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseCatalogBlock(node);
    }
  }
}

void CatalogParser::parseHrcSetsBlock(const xercesc::DOMNode* elem)
{
  addHrcSetsLocation(elem);
}

void CatalogParser::addHrcSetsLocation(const xercesc::DOMNode* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagLocation)) {
        auto attr_value = subelem->getAttribute(catLocationAttrLink);
        if (!UStr::isEmpty(attr_value)) {
          hrc_locations.emplace_back(UnicodeString(attr_value));
          logger->debug("add hrc location: '{0}'", hrc_locations.back());
        }
        else {
          logger->warn("found hrc with empty location. skip it location.");
        }
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      addHrcSetsLocation(node);
    }
  }
}

void CatalogParser::parseHrdSetsBlock(const xercesc::DOMNode* elem)
{
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), catTagHrd)) {
        auto hrd = parseHRDSetsChild(subelem);
        if (hrd) {
          hrd_nodes.push_back(std::move(hrd));
        }
      }
      continue;
    }
    if (node->getNodeType() == xercesc::DOMNode::ENTITY_REFERENCE_NODE) {
      parseHrdSetsBlock(node);
    }
  }
}

std::unique_ptr<HrdNode> CatalogParser::parseHRDSetsChild(const xercesc::DOMElement* elem)
{
  const XMLCh* xhrd_class = elem->getAttribute(catHrdAttrClass);
  const XMLCh* xhrd_name = elem->getAttribute(catHrdAttrName);
  const XMLCh* xhrd_desc = elem->getAttribute(catHrdAttrDescription);

  if (UStr::isEmpty(xhrd_class) || UStr::isEmpty(xhrd_name)) {
    logger->warn("found HRD with empty class/name. skip this record.");
    return nullptr;
  }

  auto hrd_node = std::make_unique<HrdNode>();
  hrd_node->hrd_class = UnicodeString(xhrd_class);
  hrd_node->hrd_name = UnicodeString(xhrd_name);
  hrd_node->hrd_description = UnicodeString(xhrd_desc);

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      if (xercesc::XMLString::equals(node->getNodeName(), catTagLocation)) {
        auto* subelem = static_cast<xercesc::DOMElement*>(node);
        auto attr_value = subelem->getAttribute(catLocationAttrLink);
        if (!UStr::isEmpty(attr_value)) {
          hrd_node->hrd_location.emplace_back(UnicodeString(attr_value));
          logger->debug("add hrd location '{0}' for {1}:{2}", hrd_node->hrd_location.back(),
                        hrd_node->hrd_class, hrd_node->hrd_name);
        }
        else {
          logger->warn("found hrd with empty location. skip it location.");
        }
      }
    }
  }

  if (!hrd_node->hrd_location.empty()) {
    return hrd_node;
  }
  else {
    logger->warn("skip HRD {0}:{1} - not found valid location", hrd_node->hrd_class,
                 hrd_node->hrd_name);
    return nullptr;
  }
}
