#ifndef COLORER_TEXTHRDMAPPER_H
#define COLORER_TEXTHRDMAPPER_H

#include "colorer/handlers/RegionMapper.h"
#include "colorer/handlers/TextRegion.h"

/** HRD files reader.
    HRD Files format contains mappings of HRC syntax regions into
    text indention information.
    For example, HTML indention (@c stext, @c sback, @c etext, @c eback)
    allows to create colorized HTML code.
*/
class TextHRDMapper : public RegionMapper
{
 public:
  TextHRDMapper() = default;
  ~TextHRDMapper() override = default;

  /** Loads region definitions from HRD file.
   * Multiple files could be loaded.
   */
  void loadRegionMappings(XmlInputSource& is) override;

  /**
   * Saves all loaded region defines into @c writer.
   * Note, that result document would not be equal
   * to input one, because there could be multiple input
   * documents.
   * Note, that this method writes all loaded
   * defines from all loaded HRD files.
   */

  void saveRegionMappings(Writer* writer) const override;

  /**
   * Changes specified region definition to @c rdnew
   * @param region_name Region full qualified name.
   * @param rdnew  New region definition to replace old one
   */
  void setRegionDefine(const UnicodeString& region_name, const RegionDefine* rdnew) override;
};

#endif
