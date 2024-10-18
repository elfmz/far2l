#include "colorer/parsers/CatalogParser.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/XmlInputSource.h"
#include "colorer/xml/XmlReader.h"

void CatalogParser::parse(const UnicodeString* path)
{
  COLORER_LOG_DEBUG("start parse % as catalog.xml", *path);
  hrc_locations.clear();
  hrd_nodes.clear();

  const XmlInputSource catalogXIS(*path);
  XmlReader xml(catalogXIS);
  if (!xml.parse()) {
    throw CatalogParserException(*path + UnicodeString(" parse error"));
  }
  std::list<XMLNode> nodes;
  xml.getNodes(nodes);

  if (nodes.begin()->name != catTagCatalog) {
    throw CatalogParserException("Incorrect file structure catalog.xml. Main '<catalog>' block not found at file " +
                                 *path);
  }

  parseCatalogBlock(*nodes.begin());

  COLORER_LOG_DEBUG("end parse catalog.xml");
}

void CatalogParser::parseCatalogBlock(const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == catTagHrcSets) {
      parseHrcSetsBlock(node);
    }
    else if (node.name == catTagHrdSets) {
      parseHrdSetsBlock(node);
    }
  }
}

void CatalogParser::parseHrcSetsBlock(const XMLNode& elem)
{
  addHrcSetsLocation(elem);
}

void CatalogParser::addHrcSetsLocation(const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == catTagLocation) {
      const auto& attr_value = node.getAttrValue(catLocationAttrLink);
      if (!attr_value.isEmpty()) {
        hrc_locations.push_back(attr_value);
        COLORER_LOG_DEBUG("add hrc location: '%'", attr_value);
      }
      else {
        COLORER_LOG_WARN("found hrc with empty location. skip it location.");
      }
    }
  }
}

void CatalogParser::parseHrdSetsBlock(const XMLNode& elem)
{
  for (const auto& node : elem.children) {
    if (node.name == catTagHrd) {
      auto hrd = parseHRDSetsChild(node);
      if (hrd) {
        hrd_nodes.push_back(std::move(hrd));
      }
    }
  }
}

std::unique_ptr<HrdNode> CatalogParser::parseHRDSetsChild(const XMLNode& elem)
{
  const auto& xhrd_class = elem.getAttrValue(catHrdAttrClass);
  const auto& xhrd_name = elem.getAttrValue(catHrdAttrName);

  if (xhrd_class.isEmpty() || xhrd_name.isEmpty()) {
    COLORER_LOG_WARN("found HRD with empty class/name. skip this record.");
    return nullptr;
  }

  const auto& xhrd_desc = elem.getAttrValue(catHrdAttrDescription);
  auto hrd_node = std::make_unique<HrdNode>();
  hrd_node->hrd_class = UnicodeString(xhrd_class);
  hrd_node->hrd_name = UnicodeString(xhrd_name);
  hrd_node->hrd_description = UnicodeString(xhrd_desc);

  for (const auto& node : elem.children) {
    if (node.name == catTagLocation) {
      auto attr_value = node.getAttrValue(catLocationAttrLink);
      if (!attr_value.isEmpty()) {
        hrd_node->hrd_location.emplace_back(attr_value);
        COLORER_LOG_DEBUG("add hrd location '%' for %:%", hrd_node->hrd_location.back(), hrd_node->hrd_class,
                          hrd_node->hrd_name);
      }
      else {
        COLORER_LOG_WARN("found hrd with empty location. skip it location.");
      }
    }
  }

  if (!hrd_node->hrd_location.empty()) {
    return hrd_node;
  }
  COLORER_LOG_WARN("skip HRD %:% - not found valid location", hrd_node->hrd_class, hrd_node->hrd_name);
  return nullptr;
}
