#ifndef COLORER_XMLNODE_H
#define COLORER_XMLNODE_H

#include <list>
#include <unordered_map>
#include "colorer/Common.h"

inline const UnicodeString empty_string("");

class XMLNode
{
public:
  XMLNode() = default;

  bool isExist(const UnicodeString& key) const { return attributes.find(key) != attributes.end(); }

  const UnicodeString& getAttrValue(const UnicodeString& key) const
  {
    const auto found = attributes.find(key);
    if (found == attributes.end()) {
      return empty_string;
    }
    return found->second;
  }

  UnicodeString name;  // tag name
  UnicodeString text;  // tag value ( if is a text tag )
  std::unordered_map<UnicodeString, UnicodeString> attributes;
  std::list<XMLNode> children;
};

#endif  // COLORER_XMLNODE_H
