#include <colorer/handlers/RegionMapperImpl.h>

std::vector<const RegionDefine*> RegionMapperImpl::enumerateRegionDefines() const
{
  std::vector<const RegionDefine*> r;
  r.reserve(regionDefines.size());
  for (const auto & regionDefine : regionDefines) {
    r.push_back(regionDefine.second);
  }
  return r;
}

const RegionDefine* RegionMapperImpl::getRegionDefine(const Region* region) const
{
  if (region == nullptr) {
    return nullptr;
  }
  const RegionDefine* rd = nullptr;
  if (region->getID() < regionDefinesVector.size()) {
    rd = regionDefinesVector.at(region->getID());
  }
  if (rd != nullptr) {
    return rd;
  }

  if (regionDefinesVector.size() < region->getID() + 1) {
    regionDefinesVector.resize(region->getID() * 2);
  }

  auto rd_new = regionDefines.find(region->getName());
  if (rd_new != regionDefines.end()) {
    regionDefinesVector.at(region->getID()) = rd_new->second;
    return rd_new->second;
  }

  if (region->getParent()) {
    rd = getRegionDefine(region->getParent());
    regionDefinesVector.at(region->getID()) = rd;
  }
  return rd;
}

/** Returns region mapping by it's full qualified name.
*/
const RegionDefine* RegionMapperImpl::getRegionDefine(const String &name) const
{
  auto tp = regionDefines.find(name);
  if (tp != regionDefines.end()) {
    return tp->second;
  }
  return nullptr;
}



