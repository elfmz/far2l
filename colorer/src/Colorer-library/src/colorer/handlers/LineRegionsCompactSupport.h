#ifndef _COLORER_LINEREGIONSCOMPACTSUPPORT_H_
#define _COLORER_LINEREGIONSCOMPACTSUPPORT_H_

#include <colorer/handlers/LineRegionsSupport.h>

/** Compact Region store implementation based on LineRegionsSupport class.
    This class guarantees non-interlaced creation of LineRegion structures in line.
    Client application can use this class, if it depends on
    compact style of regions layout.
    All regions are compacted, except those, marked with special field.
    @ingroup colorer_handlers
*/
class LineRegionsCompactSupport : public LineRegionsSupport
{
public:
  LineRegionsCompactSupport();
  ~LineRegionsCompactSupport();
protected:
  /** This method compacts regions while
     adding them into list structure
  */
  void addLineRegion(size_t lno, LineRegion* ladd);
};

#endif



