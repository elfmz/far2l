#include "colorer/handlers/StyledHRDMapper.h"
#include "colorer/Exception.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/XmlReader.h"

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
      regionDefines.try_emplace(name, std::move(rdef));
    }
  }
}

void StyledHRDMapper::saveRegionMappings(Writer* writer) const
{
  writer->write(u"<?xml version=\"1.0\"?>\n");

  for (const auto& [key, value] : regionDefines) {
    const StyledRegion* rdef = StyledRegion::cast(value.get());
    constexpr auto size_temporary = 256;
    char temporary[size_temporary];
    writer->write(u"\t<define name='" + key + u"'");
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
    writer->write(u"/>\n");
  }

  writer->write(u"</hrd>\n");
}

void StyledHRDMapper::setRegionDefine(const UnicodeString& region_name, const RegionDefine* rd)
{
  if (!rd) {
    return;
  }

  const StyledRegion* new_region = StyledRegion::cast(rd);
  auto rd_new = std::make_unique<StyledRegion>(*new_region);

  const auto rd_old_it = regionDefines.find(region_name);
  if (rd_old_it == regionDefines.end()) {
    regionDefines.try_emplace(region_name, std::move(rd_new));
  }
  else {
    rd_old_it->second = std::move(rd_new);
  }
}
