#ifndef COLORER_STYLEDREGION_H
#define COLORER_STYLEDREGION_H

#include "colorer/handlers/RegionDefine.h"

/**
 * Contains information about region mapping into real colors.
 * These mappings are stored in HRD files and processed by StyledHRDMapper class.
 */
class StyledRegion : public RegionDefine
{
 public:
  enum Style { RD_NONE = 0, RD_BOLD = 1, RD_ITALIC = 2, RD_UNDERLINE = 4, RD_STRIKEOUT = 8 };

  /** Is foreground value assigned? */
  bool isForeSet {false};
  /** Is background value assigned? */
  bool isBackSet {false};
  /** Foreground color of region */
  unsigned int fore {0};
  /** Background color of region */
  unsigned int back {0};
  /** Bit mask of region's style (bold, italic, underline) */
  unsigned int style {RD_NONE};

  /** Common constructor */
  StyledRegion(bool isForeSet, bool isBackSet, unsigned int fore, unsigned int back, unsigned int style);

  /** Empty constructor */
  StyledRegion();

  /** Copy constructor.
   * Clones all values including region reference.
   */
  StyledRegion(const StyledRegion& rd);
  StyledRegion& operator=(const StyledRegion& rd);

  ~StyledRegion() override = default;

  /** Static method, used to cast RegionDefine class into StyledRegion class.
   * @throw Exception If casing is not available.
   */
  static const StyledRegion* cast(const RegionDefine* rd);

  /** Completes region define with its parent values.
   *  The values only replaced, are these, which are empty
   *  in this region define. Style is replaced using OR operation.
   */
  void assignParent(const RegionDefine* parent) override;

  /**
   * Direct assign of all passed @c rd values.
   * Do not assign region reference.
   */
  void setValues(const RegionDefine* region_define) override;

  [[nodiscard]]
  RegionDefine* clone() const override;
};

#endif
