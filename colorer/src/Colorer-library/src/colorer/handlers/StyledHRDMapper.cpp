#include "colorer/handlers/StyledHRDMapper.h"
#include "colorer/Exception.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/XmlReader.h"

StyledHRDMapper::~StyledHRDMapper()
{
  regionDefines.clear();
}

void StyledHRDMapper::loadRegionMappings(XmlInputSource& is)
{
  XmlReader xml(is);
  if (!xml.parse()) {
    throw Exception("Error loading HRD file '" + is.getPath() + "'");
  }
  std::list<XMLNode> nodes;
  xml.getNodes(nodes);

  if (nodes.begin()->name != hrdTagHrd) {
    throw Exception("Incorrect hrd-file structure. Main '<hrd>' block not found. Current file " + is.getPath());
  }

  for (const auto& node : nodes.begin()->children) {
    if (node.name == hrdTagAssign) {
      const auto& name = node.getAttrValue(hrdAssignAttrName);
      if (name.isEmpty()) {
        continue;
      }

      auto rd_new = regionDefines.find(name);
      if (rd_new != regionDefines.end()) {
        COLORER_LOG_WARN("Duplicate region name '%' in file '%'. Previous value replaced.", name, is.getPath());
        regionDefines.erase(rd_new);
      }

      unsigned int fore = 0;
      bool bfore = false;
      const auto& sval = node.getAttrValue(hrdAssignAttrFore);
      if (!sval.isEmpty()) {
        bfore = UStr::HexToUInt(sval, &fore);
      }

      unsigned int back = 0;
      bool bback = false;
      const auto& sval2 = node.getAttrValue(hrdAssignAttrBack);
      if (!sval2.isEmpty()) {
        bback = UStr::HexToUInt(sval2, &back);
      }

      unsigned int style = 0;
      const auto& sval3 = node.getAttrValue(hrdAssignAttrStyle);
      if (!sval3.isEmpty()) {
        UStr::HexToUInt(sval3, &style);
      }

      auto rdef = std::make_unique<StyledRegion>(bfore, bback, fore, back, style);
      regionDefines.emplace(name, std::move(rdef));
    }
  }
}

/** Writes all currently loaded region definitions into
    XML file. Note, that this method writes all loaded
    defines from all loaded HRD files.
*/
void StyledHRDMapper::saveRegionMappings(Writer* writer) const
{
  writer->write("<?xml version=\"1.0\"?>\n");
  for (const auto& regionDefine : regionDefines) {
    const StyledRegion* rdef = StyledRegion::cast(regionDefine.second.get());
    char temporary[256];
    constexpr auto size_temporary = std::size(temporary);
    writer->write("  <define name='" + regionDefine.first + "'");
    if (rdef->isForeSet) {
      snprintf(temporary, size_temporary, " fore=\"#%06x\"", rdef->fore);
      writer->write(temporary);
    }
    if (rdef->isBackSet) {
      snprintf(temporary, size_temporary, " back=\"#%06x\"", rdef->back);
      writer->write(temporary);
    }
    if (rdef->style) {
      snprintf(temporary, size_temporary, " style=\"%u\"", rdef->style);
      writer->write(temporary);
    }
    writer->write("/>\n");
  }
  writer->write("\n</hrd>\n");
}

/** Adds or replaces region definition */
void StyledHRDMapper::setRegionDefine(const UnicodeString& name, const RegionDefine* rd)
{
  if (!rd)
    return;

  const StyledRegion* new_region = StyledRegion::cast(rd);
  auto rd_new = std::make_unique<StyledRegion>(*new_region);

  auto rd_old_it = regionDefines.find(name);
  if (rd_old_it == regionDefines.end()) {
    regionDefines.emplace(name, std::move(rd_new));
  }
  else {
    rd_old_it->second = std::move(rd_new);
  }
}
