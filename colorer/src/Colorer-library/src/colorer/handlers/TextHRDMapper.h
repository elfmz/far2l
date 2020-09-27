#ifndef _COLORER_TEXTHRDMAPPER_H_
#define _COLORER_TEXTHRDMAPPER_H_

#include <colorer/io/Writer.h>
#include <colorer/handlers/RegionMapperImpl.h>
#include <colorer/handlers/TextRegion.h>
#include <colorer/xml/XmlInputSource.h>

/** HRD files reader.
    HRD Files format contains mappings of HRC syntax regions into
    text indention information.
    For example, HTML indention (@c stext, @c sback, @c etext, @c eback)
    allows to create colorized HTML code.

    @ingroup colorer_handlers
*/
class TextHRDMapper : public RegionMapperImpl
{
public:
  TextHRDMapper();
  ~TextHRDMapper();

  /**
   * Loads region defines from @c is InputSource
   */
  void  loadRegionMappings(XmlInputSource* is);

  /**
   * Saves all loaded region defines into @c writer.
   * Note, that result document would not be equal
   * to input one, because there could be multiple input
   * documents.
   */
  void  saveRegionMappings(Writer* writer) const;

  /**
   * Changes specified region definition to @c rdnew
   * @param region Region full qualified name.
   * @param rdnew  New region definition to replace old one
   */
  void  setRegionDefine(const String &region, const RegionDefine* rdnew);
};

#endif


