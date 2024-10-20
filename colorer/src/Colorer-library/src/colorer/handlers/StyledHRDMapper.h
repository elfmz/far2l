#ifndef COLORER_STYLEDHRDMAPPER_H
#define COLORER_STYLEDHRDMAPPER_H

#include "colorer/handlers/RegionMapper.h"
#include "colorer/handlers/StyledRegion.h"

/** HRD files reader.
    HRD Files format contains mappings of HRC syntax regions into
    real coloring information. Each record in HRD (RegionDefine) can contain
    information about region color (@c fore, @c back) and about its style
    (@c style).
*/
class StyledHRDMapper : public RegionMapper
{
 public:
  StyledHRDMapper() = default;
  ~StyledHRDMapper() override = default;

  /** Loads region defines from @c is InputSource */
  void loadRegionMappings(XmlInputSource& is) override;

  /** Saves all loaded region defines into @c writer.
   *  Note, that result document would not be equal to input one,
   *   because there could be multiple input documents.
   *  Note, that this method writes all loaded
   *  defines from all loaded HRD files.
   */
  void saveRegionMappings(Writer* writer) const override;

  /** Changes specified region definition to @c rdnew
   *  @param region Region full qualified name.
   *  @param rdnew  New region definition to replace old one
   */
  void setRegionDefine(const UnicodeString& region_name, const RegionDefine* rdnew) override;
};

#endif
