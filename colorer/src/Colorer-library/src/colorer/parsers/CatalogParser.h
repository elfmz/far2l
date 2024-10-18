#ifndef COLORER_CATALOGPARSER_H
#define COLORER_CATALOGPARSER_H

#include <vector>
#include "colorer/Common.h"
#include "colorer/Exception.h"
#include "colorer/parsers/HrdNode.h"
#include "colorer/xml/XMLNode.h"

class CatalogParser
{
 public:
  CatalogParser() = default;
  ~CatalogParser() = default;

  void parse(const UnicodeString* path);
  static std::unique_ptr<HrdNode> parseHRDSetsChild(const XMLNode& elem);

  std::vector<UnicodeString> hrc_locations;
  std::vector<std::unique_ptr<HrdNode>> hrd_nodes;

  CatalogParser(CatalogParser const&) = delete;
  CatalogParser& operator=(CatalogParser const&) = delete;
  CatalogParser(CatalogParser&&) = delete;
  CatalogParser& operator=(CatalogParser&&) = delete;

 private:
  void parseCatalogBlock(const XMLNode& elem);
  void parseHrcSetsBlock(const XMLNode& elem);
  void addHrcSetsLocation(const XMLNode& elem);
  void parseHrdSetsBlock(const XMLNode& elem);
};

class CatalogParserException final : public Exception
{
 public:
  explicit CatalogParserException(const UnicodeString& msg) noexcept : Exception("[CatalogParserException] " + msg) {}
};

#endif  // COLORER_CATALOGPARSER_H
