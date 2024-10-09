#include "colorer/handlers/TextHRDMapper.h"
#include "colorer/Exception.h"
#include "colorer/base/XmlTagDefs.h"
#include "colorer/xml/XmlReader.h"

TextHRDMapper::~TextHRDMapper()
{
  regionDefines.clear();
}

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
      std::shared_ptr<const UnicodeString> stext;
      std::shared_ptr<const UnicodeString> etext;
      std::shared_ptr<const UnicodeString> sback;
      std::shared_ptr<const UnicodeString> eback;
      const auto& sval = node.getAttrValue(hrdAssignAttrSText);
      if (!sval.isEmpty()) {
        stext = std::make_unique<UnicodeString>(sval);
      }
      const auto& sval2 = node.getAttrValue(hrdAssignAttrEText);
      if (!sval2.isEmpty()) {
        etext = std::make_unique<UnicodeString>(sval2);
      }
      const auto& sval3 = node.getAttrValue(hrdAssignAttrSBack);
      if (!sval3.isEmpty()) {
        sback = std::make_unique<UnicodeString>(sval3);
      }
      const auto& sval4 = node.getAttrValue(hrdAssignAttrEBack);
      if (!sval4.isEmpty()) {
        eback = std::make_unique<UnicodeString>(sval4);
      }

      auto rdef = std::make_unique<TextRegion>(stext, etext, sback, eback);
      regionDefines.emplace(name, std::move(rdef));
    }
  }
}

void TextHRDMapper::saveRegionMappings(Writer* writer) const
{
  writer->write("<?xml version=\"1.0\"?>\n");
  for (const auto& regionDefine : regionDefines) {
    const TextRegion* rdef = TextRegion::cast(regionDefine.second.get());
    writer->write("  <define name='" + regionDefine.first + "'");
    if (rdef->start_text != nullptr) {
      writer->write(" start_text='" + *rdef->start_text + "'");
    }
    if (rdef->end_text != nullptr) {
      writer->write(" end_text='" + *rdef->end_text + "'");
    }
    if (rdef->start_back != nullptr) {
      writer->write(" start_back='" + *rdef->start_back + "'");
    }
    if (rdef->end_back != nullptr) {
      writer->write(" end_back='" + *rdef->end_back + "'");
    }
    writer->write("/>\n");
  }
  writer->write("\n</hrd>\n");
}

void TextHRDMapper::setRegionDefine(const UnicodeString& name, const RegionDefine* rd)
{
  if (!rd)
    return;

  const TextRegion* rd_new = TextRegion::cast(rd);
  auto new_region = std::make_unique<TextRegion>(*rd_new);

  auto rd_old_it = regionDefines.find(name);
  if (rd_old_it == regionDefines.end()) {
    regionDefines.emplace(name, std::move(new_region));
  }
  else {
    rd_old_it->second = std::move(new_region);
  }
}
