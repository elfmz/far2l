#ifndef _COLORER_HRDNODE_H_
#define _COLORER_HRDNODE_H_

#include <colorer/Common.h>
#include <vector>

class HRDNode
{
public:
  HRDNode() {};
  ~HRDNode()
  {
    hrd_location.clear();
  }

  SString hrd_class;
  SString hrd_name;
  SString hrd_description;
  std::vector<SString> hrd_location;
};

#endif //_COLORER_HRDNODE_H_


