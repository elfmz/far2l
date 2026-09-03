#ifndef COLORER_XMLNODE_H
#define COLORER_XMLNODE_H

#include <list>
#include <unordered_map>
#include "colorer/Common.h"

class XMLNode
{
 public:
  XMLNode() = default;

  bool isExist(const UnicodeString& key) const;

  const UnicodeString& getAttrValue(const UnicodeString& key) const;
  UnicodeString name;  // tag name
  UnicodeString text;  // tag value ( if is a text tag )
  std::unordered_map<UnicodeString, UnicodeString> attributes;
  std::list<XMLNode> children;
};

#endif  // COLORER_XMLNODE_H
