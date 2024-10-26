#ifndef COLORER_REGION_H
#define COLORER_REGION_H

#include <utility>
#include "colorer/Common.h"

/**
  HRC Region implementation.
  Contains information about HRC Region and it attributes.
*/
class Region
{
 public:
  /** Full Qualified region name (<code>def:Text</code> for example) */
  [[nodiscard]]
  virtual const UnicodeString& getName() const
  {
    return name;
  }

  /** Region description */
  [[nodiscard]]
  virtual const UnicodeString& getDescription() const
  {
    return *description;
  }

  /** Direct region ancestor (parent) */
  [[nodiscard]]
  virtual const Region* getParent() const
  {
    return parent;
  }

  /** Quick access region id (incremental) */
  [[nodiscard]]
  virtual size_t getID() const
  {
    return id;
  }

  /** Checks if region has the specified parent in all of its ancestors.
      This method is useful to check if region has specified parent,
      and use this information, as region type specification.
      For example, <code>def:Comment</code> has <code>def:Syntax</code> parent,
      so, some syntax checking can be made with its content.
  */
  bool hasParent(const Region* region) const
  {
    auto elem = this;
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
    Used only by HrcLibrary.
  */
  Region(UnicodeString _name, const UnicodeString* _description, const Region* _parent, const size_t _id)
      : name(std::move(_name)),parent(_parent), id(_id)
  {
    if (_description != nullptr) {
      description = std::make_unique<UnicodeString>(*_description);
    }
  }

  virtual ~Region() = default;

  Region(Region&&) = delete;
  Region(const Region&) = delete;
  Region& operator=(const Region&) = delete;
  Region& operator=(Region&&) = delete;

 protected:
  UnicodeString name;
  uUnicodeString description;
  const Region* parent;

  /** unique id of Region */
  size_t id;
};

#endif  // COLORER_REGION_H
