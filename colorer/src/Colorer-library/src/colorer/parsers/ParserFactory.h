#ifndef _COLORER_PARSERFACTORY_H_
#define _COLORER_PARSERFACTORY_H_

#include <colorer/TextParser.h>
#include <colorer/HRCParser.h>
#include <colorer/parsers/HRDNode.h>
#include <colorer/handlers/StyledHRDMapper.h>
#include <colorer/handlers/TextHRDMapper.h>

/**
 * Maintains catalog of HRC and HRD references.
 * This class searches and loads <code>catalog.xml</code> file
 * and creates HRCParser, StyledHRDMapper, TextHRDMapper and TextParser instances
 * with information, loaded from specified sources.
 *
 * If no path were passed to it's constructor,
 * it uses the next search order to find 'catalog.xml' file:
 *
 * - win32 systems:
 *   - image_start_dir, image_start_dir\..\
 *   - %COLORER5CATALOG%
 *   - %HOMEDRIVE%%HOMEPATH%\.colorer5catalog
 *
 * - unix/macos systems:
 *   - $COLORER5CATALOG
 *   - $HOME/.colorer5catalog
 *   - $HOMEPATH/.colorer5catalog
 *   - /usr/share/colorer/catalog.xml
 *   - /usr/local/share/colorer/catalog.xml
 *
 * @note
 *   - \%NAME%, \$NAME - Environment variable of the current process.
 *   - image_start_dir - Directory, where current image was started.
 *
 * @ingroup colorer
 */
class ParserFactory
{
public:

  /**
   * ParserFactory Constructor.
   * Searches for catalog.xml in the set of predefined locations
   * @throw ParserFactoryException If can't find catalog at any of standard locations.
   */
  ParserFactory();

  virtual ~ParserFactory();

  static const char* getVersion();

  /**
   * Enumerates all declared hrd classes
   */
  std::vector<SString> enumHRDClasses();

  /**
   * Enumerates all declared hrd instances of specified class
   */
  std::vector<const HRDNode*> enumHRDInstances(const String &classID);

  const HRDNode* getHRDNode(const String &classID, const String &nameID);
  /**
   * Creates and loads HRCParser instance from catalog.xml file.
   * This method can detect directory entries, and sequentally load their
   * contents into created HRCParser instance.
   * In other cases it uses InputSource#newInstance() method to
   * create input data stream.
   * Only one HRCParser instance is created for each ParserFactory instance.
   */
  HRCParser*  getHRCParser() const;

  /**
   * Creates TextParser instance
   */
  TextParser* createTextParser();

  /**
   * Creates RegionMapper instance and loads specified hrd files into it.
   * @param classID Class identifier of loaded hrd instance.
   * @param nameID  Name identifier of loaded hrd instances.
   * @throw ParserFactoryException If method can't find specified pair of
   *         class and name IDs in catalog.xml file
   */
  StyledHRDMapper* createStyledMapper(const String* classID, const String* nameID);
  /**
   * Creates RegionMapper instance and loads specified hrd files into it.
   * It uses 'text' class by default.
   * @param nameID  Name identifier of loaded hrd instances.
   * @throw ParserFactoryException If method can't find specified pair of
   *         class and name IDs in catalog.xml file
   */
  TextHRDMapper* createTextMapper(const String* nameID);

  size_t countHRD(const String &classID);

  /**
  * @param catalog_path Path to catalog.xml file. If null,
  *        standard search method is used.
  * @throw ParserFactoryException If can't load specified catalog.
  */
  void loadCatalog(const String* catalog_path);
  void addHrd(std::unique_ptr<HRDNode> hrd);
private:

  SString searchCatalog() const;
  void getPossibleCatalogPaths(std::vector<SString> &paths) const;

  void parseCatalog(const SString &catalog_path);

  void loadHrc(const String* hrc_path, const String* base_path) const;

  SString base_catalog_path;
  std::vector<SString> hrc_locations;
  std::unordered_map<SString, std::unique_ptr<std::vector<std::unique_ptr<HRDNode>>>> hrd_nodes;

  HRCParser* hrc_parser;

  ParserFactory(const ParserFactory &) = delete;
  void operator=(const ParserFactory &) = delete;
};

#endif


