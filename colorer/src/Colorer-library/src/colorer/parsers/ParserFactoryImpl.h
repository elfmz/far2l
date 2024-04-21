#ifndef COLORER_PARSERFACTORYIMPL_H
#define COLORER_PARSERFACTORYIMPL_H

#include "colorer/HrcLibrary.h"
#include "colorer/ParserFactory.h"
#include "colorer/TextParser.h"
#include "colorer/handlers/StyledHRDMapper.h"
#include "colorer/handlers/TextHRDMapper.h"
#include "colorer/parsers/HrdNode.h"

class ParserFactory::Impl
{
 public:
  Impl();
  ~Impl();

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl& e) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  void loadCatalog(const UnicodeString* catalog_path);
  void loadHrcPath(const UnicodeString& location);
  [[nodiscard]] HrcLibrary& getHrcLibrary() const;
  static std::unique_ptr<TextParser> createTextParser();
  std::unique_ptr<StyledHRDMapper> createStyledMapper(const UnicodeString* classID,
                                                      const UnicodeString* nameID);
  std::unique_ptr<TextHRDMapper> createTextMapper(const UnicodeString* nameID);

  /**
   * Enumerates all declared hrd classes
   */
  [[maybe_unused]] std::vector<UnicodeString> enumHrdClasses();

  /**
   * Enumerates all declared hrd instances of specified class
   */
  std::vector<const HrdNode*> enumHrdInstances(const UnicodeString& classID);

  const HrdNode& getHrdNode(const UnicodeString& classID, const UnicodeString& nameID);

  void addHrd(std::unique_ptr<HrdNode> hrd);

 private:
  void parseCatalog(const UnicodeString& catalog_path);
  void loadHrc(const UnicodeString& hrc_path, const UnicodeString* base_path) const;
  void fillMapper(const UnicodeString& classID, const UnicodeString* nameID, RegionMapper& mapper);

  uUnicodeString base_catalog_path;
  std::vector<UnicodeString> hrc_locations;
  std::unordered_map<UnicodeString, std::unique_ptr<std::vector<std::unique_ptr<HrdNode>>>>
      hrd_nodes;

  HrcLibrary* hrc_library;
};

#endif  // COLORER_PARSERFACTORYIMPL_H
