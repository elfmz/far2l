#ifndef COLORER_PARSERFACTORY_H
#define COLORER_PARSERFACTORY_H

#include "colorer/Common.h"
#include "colorer/Exception.h"
#include "colorer/HrcLibrary.h"
#include "colorer/TextParser.h"
#include "colorer/common/spimpl.h"
#include "colorer/handlers/StyledHRDMapper.h"
#include "colorer/handlers/TextHRDMapper.h"
#include "colorer/parsers/HrdNode.h"

/**
 * Maintains catalog of HRC and HRD references.
 * This class searches and loads <code>catalog.xml</code> file
 * and creates HrcLibrary, StyledHRDMapper, TextHRDMapper and TextParser instances
 * with information, loaded from specified sources.
 *
 */
class ParserFactory
{
 public:
  /**
   * ParserFactory Constructor.
   */
  ParserFactory();

  /**
   * Load catalog.xml.
   * @param catalog_path Path to catalog.xml file. If null, searches for catalog.xml in the set of predefined locations
   * @throw ParserFactoryException If can't load specified catalog.
   */
  void loadCatalog(const UnicodeString* catalog_path);

  /**
   * Load prototypes from the specified source.
   * @param location Path to hrc file or folder with hrc files
   */
  void loadHrcPath(const UnicodeString* location);

  void loadHrcSettings(const UnicodeString* location, bool user_defined);

  void loadHrdPath(const UnicodeString* location);

  /**
   * Creates and loads HrcLibrary instance from catalog.xml file.
   * This method can detect directory entries, and sequentially load their
   * contents into created HrcLibrary instance.
   * In other cases it uses InputSource#newInstance() method to
   * create input data stream.
   * Only one HrcLibrary instance is created for each ParserFactory instance.
   */
  [[nodiscard]]
  HrcLibrary& getHrcLibrary() const;

  /**
   * Creates TextParser instance
   */
  std::unique_ptr<TextParser> createTextParser();
  /**
   * Creates RegionMapper instance and loads specified hrd files into it.
   * @param classID Class identifier of loaded hrd instance.
   * @param nameID  Name identifier of loaded hrd instances.
   * @throw ParserFactoryException If method can't find specified pair of
   *         class and name IDs in catalog.xml file
   */
  std::unique_ptr<StyledHRDMapper> createStyledMapper(const UnicodeString* classID, const UnicodeString* nameID);

  /**
   * Creates RegionMapper instance and loads specified hrd files into it.
   * It uses 'text' class by default.
   * @param nameID  Name identifier of loaded hrd instances.
   * @throw ParserFactoryException If method can't find specified pair of
   *         class and name IDs in catalog.xml file
   */
  std::unique_ptr<TextHRDMapper> createTextMapper(const UnicodeString* nameID);

  std::vector<const HrdNode*> enumHrdInstances(const UnicodeString& classID);
  void addHrd(std::unique_ptr<HrdNode> hrd);
  const HrdNode& getHrdNode(const UnicodeString& classID, const UnicodeString& nameID);

 private:
  class Impl;

  spimpl::unique_impl_ptr<Impl> pimpl;
};

/** Exception, thrown by ParserFactory class methods.
 *  Indicates some (mostly fatal) errors in loading of catalog file (catalog.xml), or in creating
 * parsers objects.
 *   @ingroup colorer
 */
class ParserFactoryException : public Exception
{
 public:
  explicit ParserFactoryException(const UnicodeString& msg) noexcept : Exception("[ParserFactoryException] " + msg) {}
};

#endif  // COLORER_PARSERFACTORY_H
