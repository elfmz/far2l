#ifndef _COLORER_TEXTREGION_H_
#define _COLORER_TEXTREGION_H_

#include "colorer/handlers/RegionDefine.h"
#include <memory>

/**
 * Contains information about region mapping into textual prefix/suffix.
 * These mappings are stored in HRD files.
 * @ingroup colorer_handlers
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

  /**
   * Initial constructor
   */
  TextRegion(std::shared_ptr<const UnicodeString>& _start_text, std::shared_ptr<const UnicodeString>& _end_text,
             std::shared_ptr<const UnicodeString>& _start_back, std::shared_ptr<const UnicodeString>& _end_back);
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
   * Assigns region define with it's parent values.
   * All fields are to be replaced, if they are null-ed.
   */
  void assignParent(const RegionDefine* _parent) override;
  /**
   * Direct assign of all passed @c rd values.
   * Do not assign region reference.
   */
  void setValues(const RegionDefine* _rd) override;

  [[nodiscard]] RegionDefine* clone() const override;
};

#endif
