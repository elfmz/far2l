#ifndef _COLORER_LINEREGION_H_
#define _COLORER_LINEREGION_H_

#include <colorer/handlers/RegionDefine.h>
#include <colorer/handlers/StyledRegion.h>
#include <colorer/handlers/TextRegion.h>
#include <colorer/Scheme.h>

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
  int start, end;
  /** Reference to region's HRC scheme */
  const Scheme* scheme;
  /** Previous and next links to ranged region in this line.
      First region of each line contains reference to it's last
      region in prev field.
      If @c next field is null, this is a last region in line.
  */
  LineRegion* next, *prev;
  /** Special meaning marker. Generally this is used to inform
      application about paired regions, which are invisible during
      ordinary text drawing.
  */
  bool special;

  /** Transforms this region's reference into styled region define
      and returns new pointer.
  */
  const StyledRegion* styled()
  {
    return StyledRegion::cast(rdef);
  }
  /** Transforms this region's reference into text region define
      and returns new pointer.
  */
  const TextRegion* texted()
  {
    return TextRegion::cast(rdef);
  }
  /** Copy operator */
  LineRegion& operator=(const LineRegion& lr)
  {
    start = lr.start;
    end   = lr.end;
    scheme = lr.scheme;
    region = lr.region;
    special = lr.special;
    rdef = nullptr;
    if (lr.rdef != nullptr) {
      rdef = lr.rdef->clone();
    }
    return *this;
  }
  /** Clears all fields */
  LineRegion()
  {
    next = prev = nullptr;
    start = end = 0;
    scheme = nullptr;
    region = nullptr;
    rdef = nullptr;
    special = false;
  }
  /** Copy constructor.
      Do not copies next and prev pointers.
  */
  LineRegion(const LineRegion& lr)
  {
    //_next = lr._next;
    //_prev = lr._prev;
    operator=(lr);
  }
  ~LineRegion()
  {
    delete rdef;
  }
};

#endif



