#ifndef COLORER_REGIONDEFINE_H
#define COLORER_REGIONDEFINE_H

/**
 * Object contains information about region mapping into real colors or other properties.
 * This class represents abstract mapping information and declares required methods
 * to be implemented in its subclasses.
 */
class RegionDefine
{
 public:
  /**
   * Enumeration to distinguish different types of region mapping
   * Do not use RTTI because of compatibility problems
   */
  enum class RegionDefineType {
    UNKNOWN_REGION = 0,
    STYLED_REGION = 1,
    TEXT_REGION = 2,
  };

  /**
   * Class type identifier
   */
  RegionDefineType type {RegionDefineType::UNKNOWN_REGION};

  /**
   * Completes region define values with its parent values.
   * If region define has some incomplete information (fe some
   * transparent fields), these methods completes them with
   * passed parent's values.
   */
  virtual void assignParent(const RegionDefine* parent) = 0;

  /**
   * Direct assign of all passed @c rd values.
   * Copies all information from passed definition into
   * this region.
   */
  virtual void setValues(const RegionDefine* rd) = 0;

  /**
   * Assign operator. Clones all values.
   * Works as setValues method.
   */
  RegionDefine& operator=(const RegionDefine& rd)
  {
    if (this != &rd) {
      setValues(&rd);
    }
    return *this;
  }

  RegionDefine(const RegionDefine& rd) = delete;

  /**
   * Clones current region and creates it's duplicate.
   * To be implemented in subclasses.
   */
  [[nodiscard]]
  virtual RegionDefine* clone() const = 0;

  virtual ~RegionDefine() = default;

  RegionDefine(RegionDefine&&) = delete;
  RegionDefine& operator=(RegionDefine&&) = delete;

 protected:
  RegionDefine() = default;
};

#endif
