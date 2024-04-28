#ifndef COLORER_CATALOGPARSER_H
#define COLORER_CATALOGPARSER_H

#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "colorer/Common.h"
#include "colorer/Exception.h"
#include "colorer/parsers/HrdNode.h"

class CatalogParser
{
 public:
  CatalogParser() = default;
  ~CatalogParser() = default;

  void parse(const UnicodeString* path);
  static std::unique_ptr<HrdNode> parseHRDSetsChild(const xercesc::DOMElement* elem);

  std::vector<UnicodeString> hrc_locations;
  std::vector<std::unique_ptr<HrdNode>> hrd_nodes;

  CatalogParser(CatalogParser const&) = delete;
  CatalogParser& operator=(CatalogParser const&) = delete;
  CatalogParser(CatalogParser&&) = delete;
  CatalogParser& operator=(CatalogParser&&) = delete;

 private:
  void parseCatalogBlock(const xercesc::DOMNode* elem);
  void parseHrcSetsBlock(const xercesc::DOMNode* elem);
  void addHrcSetsLocation(const xercesc::DOMNode* elem);
  void parseHrdSetsBlock(const xercesc::DOMNode* elem);
};

class CatalogParserException : public Exception
{
 public:
  explicit CatalogParserException(const UnicodeString& msg) noexcept
      : Exception("[CatalogParserException] " + msg)
  {
  }
};

#endif  // COLORER_CATALOGPARSER_H
