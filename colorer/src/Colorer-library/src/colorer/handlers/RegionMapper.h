#ifndef _COLORER_REGIONMAPPERIMPL_H_
#define _COLORER_REGIONMAPPERIMPL_H_

#include <unordered_map>
#include <vector>
#include "colorer/Region.h"
#include "colorer/handlers/RegionDefine.h"
#include "colorer/io/Writer.h"
#include "colorer/xml/XmlInputSource.h"

/** Abstract RegionMapper.
    Stores all region mappings in hashtable and sequental vector for Region -> RegionDefine
   mappings.
    @ingroup colorer_handlers
*/
class RegionMapper
{
 public:
  RegionMapper() = default;
  virtual ~RegionMapper() = default;

  /** Loads region defines from @c is InputSource
   */
  virtual void loadRegionMappings(XmlInputSource& is) = 0;

  /** Saves all loaded region defines into @c writer.
      Note, that result document would not be equal
      to input one, because there could be multiple input
      documents.
  */
  virtual void saveRegionMappings(Writer* writer) const = 0;

  /** Changes specified region definition to @c rdnew
      @param region Region full qualified name.
      @param rdnew  New region definition to replace old one
  */

  virtual void setRegionDefine(const UnicodeString& region, const RegionDefine* rdnew) = 0;

  /** Enumerates all loaded region defines.
      @return RegionDefine with specified internal index, or null if @c idx is too big
  */
  std::vector<const RegionDefine*> enumerateRegionDefines() const;

  /**
   * Searches mapped region define value.
   * @return Region define, associated with passed @c region
   * parameter, or null if nothing found
   */
  const RegionDefine* getRegionDefine(const Region* region) const;

  /**
   * Searches mapped region define value with qualified name @c name.
   */
  const RegionDefine* getRegionDefine(const UnicodeString& name) const;

  RegionMapper(RegionMapper&&) = delete;
  RegionMapper(const RegionMapper&) = delete;
  RegionMapper& operator=(const RegionMapper&) = delete;
  RegionMapper& operator=(RegionMapper&&) = delete;

 protected:
  // all RegionDefine
  std::unordered_map<UnicodeString, std::unique_ptr<RegionDefine>> regionDefines;
  // "cache" for fast getting RegionDefine
  mutable std::vector<const RegionDefine*> regionDefinesCache;
};

#endif
