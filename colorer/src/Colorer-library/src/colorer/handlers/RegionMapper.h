#ifndef _COLORER_REGIONMAPPER_H_
#define _COLORER_REGIONMAPPER_H_

#include <colorer/Region.h>
#include <colorer/handlers/RegionDefine.h>

/**
 * Abstract interface to perform Region -> RegionDefine mappings.
 * @ingroup colorer_handlers
 */
class RegionMapper
{
public:
  /**
   * Searches mapped region define value.
   * @return Region define, associated with passed @c region
   * parameter, or null if nothing found
   */
  virtual const RegionDefine* getRegionDefine(const Region* region) const = 0;

  /**
   * Searches mapped region define value with qualified name @c name.
   */
  virtual const RegionDefine* getRegionDefine(const String &name) const = 0;

  virtual ~RegionMapper() {};
protected:
  RegionMapper() {};
};

#endif


