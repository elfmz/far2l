#ifndef _COLORER_LINEREGION_H_
#define _COLORER_LINEREGION_H_

#include "colorer/Scheme.h"
#include "colorer/handlers/RegionDefine.h"
#include "colorer/handlers/StyledRegion.h"
#include "colorer/handlers/TextRegion.h"
#include "colorer/Region.h"

/** Defines region position properties.
    These properties are created dynamically during text parsing
    and stores region's position on line and mapping
    of region into the RegionDefine instance.
    @ingroup colorer_handlers
*/
class LineRegion
{
 public:
  /** Reference to HRC region, which identifies type of this range */
  const Region* region;

  /** Reference to RegionDefine class (it's subclass).
      This reference can contain concrete information about region
      extended properties.
      Can be null, if no region mapping were defined.
  */
  RegionDefine* rdef;

  /** Start and End position of region in line */
  int start;
  int end;

  /** Reference to region's HRC scheme */
  const Scheme* scheme;

  /** Previous and next links to ranged region in this line.
      First region of each line contains reference to it's last
      region in prev field.
      If @c next field is null, this is a last region in line.
  */
  LineRegion *next;
  LineRegion *prev;

  /** Special meaning marker. Generally this is used to inform
      application about paired regions, which are invisible during
      ordinary text drawing.
  */
  bool special;

  /** Transforms this region's reference into styled region define
      and returns new pointer.
  */
  const StyledRegion* styled();

  /** Transforms this region's reference into text region define
      and returns new pointer.
  */
  const TextRegion* texted();

  /** Copy operator */
  LineRegion& operator=(const LineRegion& lr);

  /** Clears all fields */
  LineRegion();

  /** Copy constructor.
      Do not copies next and prev pointers.
  */
  LineRegion(const LineRegion& lr);

  ~LineRegion();

  void assigment(const LineRegion& lr);
};

#endif
