#ifndef _COLORER_STYLEDREGION_H_
#define _COLORER_STYLEDREGION_H_

#include <colorer/Exception.h>
#include <colorer/handlers/RegionDefine.h>

/**
 * Contains information about region mapping into real colors.
 * These mappings are stored in HRD files and processed
 * by StyledHRDMapper class.
 * @ingroup colorer_handlers
 */
class StyledRegion : public RegionDefine
{
public:
  static const int RD_BOLD;
  static const int RD_ITALIC;
  static const int RD_UNDERLINE;
  static const int RD_STRIKEOUT;

  /** Is foreground value assigned? */
  bool bfore;
  /** Is background value assigned? */
  bool bback;
  /** Foreground color of region */
  unsigned int fore;
  /** Background color of region */
  unsigned int back;
  /** Bit mask of region's style (bold, italic, underline) */
  unsigned int style;

  /** Common constructor */
  StyledRegion(bool _bfore, bool _bback, unsigned int _fore, unsigned int _back, unsigned int _style)
  {
    type = RegionDefine::STYLED_REGION;
    bfore = _bfore;
    bback = _bback;
    fore = _fore;
    back = _back;
    style = _style;
  }

  /** Empty constructor */
  StyledRegion()
  {
    type = RegionDefine::STYLED_REGION;
    bfore = bback = false;
    fore = back = 0;
    style = 0;
  }

  /** Copy constructor.
      Clones all values including region reference. */
  StyledRegion(const StyledRegion &rd)
  {
    operator=(rd);
  }

  ~StyledRegion() {}

  /** Static method, used to cast RegionDefine class into
      StyledRegion class.
      @throw Exception If casing is not available.
  */
  static const StyledRegion* cast(const RegionDefine* rd)
  {
    if (rd == nullptr) return nullptr;
    if (rd->type != RegionDefine::STYLED_REGION) throw Exception(CString("Bad type cast exception into StyledRegion"));
    const StyledRegion* sr = (const StyledRegion*)(rd);
    return sr;
  }
  /** Completes region define with it's parent values.
      The values only replaced, are these, which are empty
      in this region define. Style is replaced using OR operation.
  */
  void assignParent(const RegionDefine* _parent)
  {
    const StyledRegion* parent = StyledRegion::cast(_parent);
    if (parent == nullptr) return;
    if (!bfore) {
      fore = parent->fore;
      bfore = parent->bfore;
    }
    if (!bback) {
      back = parent->back;
      bback = parent->bback;
    }
    style = style | parent->style;
  }

  void setValues(const RegionDefine* _rd)
  {
    const StyledRegion* rd = StyledRegion::cast(_rd);
    fore  = rd->fore;
    bfore = rd->bfore;
    back  = rd->back;
    bback = rd->bback;
    style = rd->style;
    type  = rd->type;
  }

  RegionDefine* clone() const
  {
    RegionDefine* rd = new StyledRegion(*this);
    return rd;
  }
};

#endif


