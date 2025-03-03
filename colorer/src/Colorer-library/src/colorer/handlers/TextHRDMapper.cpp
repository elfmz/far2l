#include "colorer/handlers/TextHRDMapper.h"
#include "colorer/Exception.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/XmlReader.h"

void TextHRDMapper::loadRegionMappings(XmlInputSource& is)
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

      auto tp = regionDefines.find(name);
      if (tp != regionDefines.end()) {
        COLORER_LOG_WARN("Duplicate region name '%' in file '%'. Previous value replaced.", name, is.getPath());
        regionDefines.erase(tp);
      }

      const auto& stext = node.getAttrValue(hrdAssignAttrSText);
      const auto& etext = node.getAttrValue(hrdAssignAttrEText);
      const auto& sback = node.getAttrValue(hrdAssignAttrSBack);
      const auto& eback = node.getAttrValue(hrdAssignAttrEBack);

      auto rdef = std::make_unique<TextRegion>(stext, etext, sback, eback);
      regionDefines.try_emplace(name, std::move(rdef));
    }
  }
}

void TextHRDMapper::saveRegionMappings(Writer* writer) const
{
  writer->write(u"<?xml version=\"1.0\"?>\n");

  for (const auto& [key, value] : regionDefines) {
    const TextRegion* rdef = TextRegion::cast(value.get());
    writer->write(u"\t<define name='" + key + u"'");
    if (rdef->start_text != nullptr) {
      writer->write(u" start_text='" + *rdef->start_text + u"'");
    }
    if (rdef->end_text != nullptr) {
      writer->write(u" end_text='" + *rdef->end_text + u"'");
    }
    if (rdef->start_back != nullptr) {
      writer->write(u" start_back='" + *rdef->start_back + u"'");
    }
    if (rdef->end_back != nullptr) {
      writer->write(u" end_back='" + *rdef->end_back + u"'");
    }
    writer->write(u"/>\n");
  }

  writer->write(u"</hrd>\n");
}

void TextHRDMapper::setRegionDefine(const UnicodeString& region_name, const RegionDefine* rd)
{
  if (!rd)
    return;

  const TextRegion* rd_new = TextRegion::cast(rd);
  auto new_region = std::make_unique<TextRegion>(*rd_new);

  const auto rd_old_it = regionDefines.find(region_name);
  if (rd_old_it == regionDefines.end()) {
    regionDefines.try_emplace(region_name, std::move(new_region));
  }
  else {
    rd_old_it->second = std::move(new_region);
  }
}
