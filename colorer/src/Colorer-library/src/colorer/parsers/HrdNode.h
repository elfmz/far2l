#ifndef COLORER_HRDNODE_H
#define COLORER_HRDNODE_H

#include <vector>
#include "colorer/Common.h"

class HrdNode
{
 public:
  HrdNode() = default;

  UnicodeString hrd_class;
  UnicodeString hrd_name;
  UnicodeString hrd_description;
  std::vector<UnicodeString> hrd_location;
};

#endif  // COLORER_HRDNODE_H
