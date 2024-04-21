#include "colorer/handlers/RegionMapper.h"

std::vector<const RegionDefine*> RegionMapper::enumerateRegionDefines() const
{
  std::vector<const RegionDefine*> r;
  r.reserve(regionDefines.size());
  for (const auto& regionDefine : regionDefines) {
    r.push_back(regionDefine.second.get());
  }
  return r;
}

const RegionDefine* RegionMapper::getRegionDefine(const Region* region) const
{
  if (!region)
    return nullptr;

  // search in cache
  const RegionDefine* rd = nullptr;
  if (region->getID() < regionDefinesCache.size()) {
    rd = regionDefinesCache.at(region->getID());
  }
  // RegionDefine maybe not yet in cache
  if (rd != nullptr) {
    return rd;
  }

  if (regionDefinesCache.size() < region->getID() + 1) {
    regionDefinesCache.resize(region->getID() * 2);
  }

  auto rd_new = regionDefines.find(region->getName());
  if (rd_new != regionDefines.end()) {
    regionDefinesCache.at(region->getID()) = rd_new->second.get();
    return rd_new->second.get();
  }

  if (region->getParent()) {
    rd = getRegionDefine(region->getParent());
    regionDefinesCache.at(region->getID()) = rd;
  }
  return rd;
}

const RegionDefine* RegionMapper::getRegionDefine(const UnicodeString& name) const
{
  auto tp = regionDefines.find(name);
  if (tp != regionDefines.end()) {
    return tp->second.get();
  }
  return nullptr;
}
