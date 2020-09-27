#ifndef _COLORER_REGIONMAPPERIMPL_H_
#define _COLORER_REGIONMAPPERIMPL_H_

#include <vector>
#include <colorer/io/Writer.h>
#include <colorer/handlers/RegionMapper.h>
#include <colorer/handlers/RegionDefine.h>
#include <xercesc/sax/InputSource.hpp>
#include <colorer/xml/XmlInputSource.h>

/** Abstract RegionMapper.
    Stores all region mappings in hashtable and sequental vector.
    @ingroup colorer_handlers
*/
class RegionMapperImpl : public RegionMapper
{
public:
  RegionMapperImpl() {};
  ~RegionMapperImpl() {};

  /** Loads region defines from @c is InputSource
  */
  virtual void  loadRegionMappings(XmlInputSource* is) = 0;
  /** Saves all loaded region defines into @c writer.
      Note, that result document would not be equal
      to input one, because there could be multiple input
      documents.
  */
  virtual void  saveRegionMappings(Writer* writer) const = 0;
  /** Changes specified region definition to @c rdnew
      @param region Region full qualified name.
      @param rdnew  New region definition to replace old one
  */
  virtual void  setRegionDefine(const String &region, const RegionDefine* rdnew) = 0;

  /** Enumerates all loaded region defines.
      @return RegionDefine with specified internal index, or null if @c idx is too big
  */
  std::vector<const RegionDefine*> enumerateRegionDefines() const;

  const RegionDefine* getRegionDefine(const Region* region) const;
  const RegionDefine* getRegionDefine(const String &name) const;

protected:
  std::unordered_map<SString, RegionDefine*> regionDefines;
  mutable std::vector<const RegionDefine*> regionDefinesVector;

  RegionMapperImpl(const RegionMapperImpl &);
  void operator=(const RegionMapperImpl &);
};

#endif


