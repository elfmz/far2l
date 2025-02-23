#include "colorer/handlers/RegionMapper.h"

std::vector<const RegionDefine*> RegionMapper::enumerateRegionDefines() const
{
  std::vector<const RegionDefine*> result(regionDefines.size());
  for (const auto& [key, value] : regionDefines) {
    result.push_back(value.get());
  }
  return result;
}

const RegionDefine* RegionMapper::getRegionDefine(const Region* region) const
{
  if (!region) {
    return nullptr;
  }

  // search in cache
  const RegionDefine* result = nullptr;
  if (region->getID() < regionDefinesCache.size()) {
    result = regionDefinesCache.at(region->getID());
    // RegionDefine maybe not yet in cache
    if (result != nullptr) {
      return result;
    }
  }

  if (regionDefinesCache.size() < region->getID() + 1) {
    regionDefinesCache.resize(region->getID() * 2);
  }

  const auto rd_new = regionDefines.find(region->getName());
  if (rd_new != regionDefines.end()) {
    regionDefinesCache.at(region->getID()) = rd_new->second.get();
    return rd_new->second.get();
  }

  if (region->getParent()) {
    result = getRegionDefine(region->getParent());
    regionDefinesCache.at(region->getID()) = result;
  }
  return result;
}

const RegionDefine* RegionMapper::getRegionDefine(const UnicodeString& name) const
{
  const auto tp = regionDefines.find(name);
  if (tp != regionDefines.end()) {
    return tp->second.get();
  }
  return nullptr;
}
