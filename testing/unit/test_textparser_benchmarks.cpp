#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

#include "colorer/ParserFactory.h"
#include "colorer/LineSource.h"
#include "colorer/RegionHandler.h"
#include "colorer/TextParser.h"
#include "colorer/HrcLibrary.h"

#ifndef FAR2L_SOURCE_DIR
#error "FAR2L_SOURCE_DIR must be defined by CMake (see testing/unit/CMakeLists.txt)"
#endif

class VecLineSource : public LineSource {
 public:
  std::vector<UnicodeString> lines;
  UnicodeString* getLine(size_t lno) override {
    if (lno < lines.size()) {
      return &lines[lno];
    }
    return nullptr;
  }
};

class NullRegionHandler : public RegionHandler {
 public:
  void addRegion(size_t /*lno*/, UnicodeString* /*line*/, int /*sx*/, int /*ex*/,
                 const Region* /*region*/) override {}
  void enterScheme(size_t /*lno*/, UnicodeString* /*line*/, int /*sx*/, int /*ex*/,
                   const Region* /*region*/, const Scheme* /*scheme*/) override {}
  void leaveScheme(size_t /*lno*/, UnicodeString* /*line*/, int /*sx*/, int /*ex*/,
                   const Region* /*region*/, const Scheme* /*scheme*/) override {}
};

static std::string GetCatalogPath() {
  return std::string(FAR2L_SOURCE_DIR) + "/colorer/configs/base/catalog.xml";
}

TEST(TextParserBenchmarks, ParseSimpleC) {
  ParserFactory pf;
  UnicodeString catalog_path(GetCatalogPath().c_str());
  pf.loadCatalog(&catalog_path);

  VecLineSource ls;
  for (int i = 0; i < 100; ++i) {
    ls.lines.emplace_back("int main() { return 0; } // comment");
  }

  NullRegionHandler rh;
  auto parser = pf.createTextParser();
  parser->setLineSource(&ls);
  parser->setRegionHandler(&rh);

  UnicodeString type_name("c");
  FileType* ft = pf.getHrcLibrary().getFileType(&type_name);
  ASSERT_NE(ft, nullptr);
  pf.getHrcLibrary().loadFileType(ft);
  parser->setFileType(ft);

  auto t0 = std::chrono::high_resolution_clock::now();
  int parsed = parser->parse(0, 100, TextParser::TextParseMode::TPM_CACHE_OFF);
  auto t1 = std::chrono::high_resolution_clock::now();

  auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
  std::cerr << "[BENCH] TextParser parse C (100 lines): " << us << " us, parsed=" << parsed << "\n";

  EXPECT_GE(parsed, 90);
  EXPECT_LT(us, 500000);  // 500 ms generous
}

TEST(TextParserBenchmarks, ParseSimpleCpp) {
  ParserFactory pf;
  UnicodeString catalog_path(GetCatalogPath().c_str());
  pf.loadCatalog(&catalog_path);

  VecLineSource ls;
  for (int i = 0; i < 100; ++i) {
    ls.lines.emplace_back("std::vector<int> v; for (auto& x : v) { std::cout << x << \"\\n\"; }");
  }

  NullRegionHandler rh;
  auto parser = pf.createTextParser();
  parser->setLineSource(&ls);
  parser->setRegionHandler(&rh);

  UnicodeString type_name("cpp");
  FileType* ft = pf.getHrcLibrary().getFileType(&type_name);
  ASSERT_NE(ft, nullptr);
  pf.getHrcLibrary().loadFileType(ft);
  parser->setFileType(ft);

  auto t0 = std::chrono::high_resolution_clock::now();
  int parsed = parser->parse(0, 100, TextParser::TextParseMode::TPM_CACHE_OFF);
  auto t1 = std::chrono::high_resolution_clock::now();

  auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
  std::cerr << "[BENCH] TextParser parse C++ (100 lines): " << us << " us, parsed=" << parsed
            << "\n";

  EXPECT_GE(parsed, 90);
  EXPECT_LT(us, 500000);
}

TEST(TextParserBenchmarks, ParseHeavyRegexLines) {
  ParserFactory pf;
  UnicodeString catalog_path(GetCatalogPath().c_str());
  pf.loadCatalog(&catalog_path);

  VecLineSource ls;
  for (int i = 0; i < 50; ++i) {
    ls.lines.emplace_back(
        "#include <iostream> /* block comment */ int x = 123; "
        "std::string s = \"hello world\"; void foo() { }");
  }

  NullRegionHandler rh;
  auto parser = pf.createTextParser();
  parser->setLineSource(&ls);
  parser->setRegionHandler(&rh);

  UnicodeString type_name("cpp");
  FileType* ft = pf.getHrcLibrary().getFileType(&type_name);
  ASSERT_NE(ft, nullptr);
  pf.getHrcLibrary().loadFileType(ft);
  parser->setFileType(ft);

  auto t0 = std::chrono::high_resolution_clock::now();
  int parsed = parser->parse(0, 50, TextParser::TextParseMode::TPM_CACHE_OFF);
  auto t1 = std::chrono::high_resolution_clock::now();

  auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
  std::cerr << "[BENCH] TextParser parse heavy C++ (50 lines): " << us << " us, parsed=" << parsed
            << "\n";

  EXPECT_GE(parsed, 40);
  EXPECT_LT(us, 500000);
}
