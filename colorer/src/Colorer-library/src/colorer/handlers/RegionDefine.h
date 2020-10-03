#ifndef _COLORER_REGIONDEFINE_H_
#define _COLORER_REGIONDEFINE_H_

/**
 * Object contains information about region mapping into real colors or other properties.
 * This class represents abstract mapping information and declares required methods
 * to be implemented in it's subclasses.
 *
 * @ingroup colorer_handlers
 */
class RegionDefine
{
public:

  /**
  * Enumeration to distinguish different types of region mapping
  * Do not use RTTI because of compatibility problems
  *
  * @ingroup colorer_handlers
  */
  enum RegionDefineType {
    UNKNOWN_REGION = 0,
    STYLED_REGION = 1,
    TEXT_REGION = 2,
  };

  /**
   * Class type identifier
   */
  RegionDefineType type;

  /**
   * Completes region define values with it's parent values.
   * If region define has some incomplete information (fe some
   * transparent fields), this methods completes them with
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
  virtual RegionDefine &operator=(const RegionDefine &rd)
  {
    setValues(&rd);
    return *this;
  }

  /**
   * Clones current region and creates it's duplicate.
   * To be implemented in subclasses.
   */
  virtual RegionDefine* clone() const = 0;

  /** Default Destructor */
  virtual ~RegionDefine() {};
};

#endif


