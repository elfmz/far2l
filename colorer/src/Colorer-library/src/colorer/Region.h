#ifndef _COLORER_REGION_H_
#define _COLORER_REGION_H_

#include <colorer/Common.h>

/**
  HRC Region implementation.
  Contains information about HRC Region and it attributes:
  <ul>
    <li>name
    <li>description
    <li>parent
  </ul>
  @ingroup colorer
*/
class Region
{
public:
  /** Full Qualified region name (<code>def:Text</code> for example) */
  virtual const String* getName() const
  {
    return name;
  }
  /** Region description */
  virtual const String* getDescription() const
  {
    return description;
  }
  /** Direct region ancestor (parent) */
  virtual const Region* getParent() const
  {
    return parent;
  }
  /** Quick access region id (incrementable) */
  virtual int getID() const
  {
    return id;
  }
  /** Checks if region has the specified parent in all of it's ancestors.
      This method is useful to check if region has specified parent,
      and use this information, as region type specification.
      For example, <code>def:Comment</code> has <code>def:Syntax</code> parent,
      so, some syntax checking can be made with it's content.
  */
  bool hasParent(const Region* region) const
  {
    const Region* elem = this;
    while (elem != nullptr) {
      if (region == elem) {
        return true;
      }
      elem = elem->getParent();
    }
    return false;
  }
  /**
    Basic constructor.
    Used only by HRCParser.
  */
  Region(const String* _name, const String* _description, const Region* _parent, int _id)
  {
    name = new SString(_name);
    description = nullptr;
    if (_description != nullptr) {
      description = new SString(_description);
    }
    parent = _parent;
    id = _id;
  }

  virtual ~Region()
  {
    delete name;
    delete description;
  }

protected:
  /** Internal members */
  String* name, *description;
  const Region* parent;
  int id;
};

#endif


