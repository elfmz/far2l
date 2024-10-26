#include "colorer/xml/XMLNode.h"

const UnicodeString& XMLNode::getAttrValue(const UnicodeString& key) const
{
  static const UnicodeString empty_string(u"");

  const auto found = attributes.find(key);
  if (found == attributes.end()) {
    return empty_string;
  }
  return found->second;
}

bool XMLNode::isExist(const UnicodeString& key) const
{
  return attributes.find(key) != attributes.end();
}