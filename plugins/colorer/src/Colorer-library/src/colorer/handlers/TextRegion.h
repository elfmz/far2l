#ifndef COLORER_TEXTREGION_H
#define COLORER_TEXTREGION_H

#include "colorer/Common.h"
#include "colorer/handlers/RegionDefine.h"

/**
 * Contains information about region mapping into textual prefix/suffix.
 * These mappings are stored in HRD files.
 */
class TextRegion : public RegionDefine
{
 public:
  /**
   * Text wrapping information.
   * Pointers are managed externally.
   */
  std::shared_ptr<const UnicodeString> start_text;
  std::shared_ptr<const UnicodeString> end_text;
  std::shared_ptr<const UnicodeString> start_back;
  std::shared_ptr<const UnicodeString> end_back;

  /** Common constructor */
  TextRegion(const UnicodeString& start_text, const UnicodeString& end_text, const UnicodeString& start_back,
             const UnicodeString& end_back);

  /** Empty constructor */
  TextRegion();

  /**
   * Copy constructor.
   * Clones all values including region reference
   */
  TextRegion(const TextRegion& rd);
  TextRegion& operator=(const TextRegion& rd);

  ~TextRegion() override = default;

  /**
   * Static method, used to cast RegionDefine class into TextRegion class.
   * @throw Exception If casing is not available.
   */
  static const TextRegion* cast(const RegionDefine* rd);

  /**
   * Assigns region define with its parent values.
   * All fields are to be replaced, if they are null-ed.
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
