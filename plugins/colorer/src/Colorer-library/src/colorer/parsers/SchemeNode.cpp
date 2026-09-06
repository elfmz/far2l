#include "colorer/parsers/SchemeNode.h"

SchemeNodeInherit::~SchemeNodeInherit()
{
  for (auto it : virtualEntryVector) {
    delete it;
  }
}
