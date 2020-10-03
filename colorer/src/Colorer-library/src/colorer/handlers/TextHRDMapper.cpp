#include <stdio.h>
#include <colorer/handlers/TextHRDMapper.h>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <colorer/xml/XmlTagDefs.h>

TextHRDMapper::TextHRDMapper() {}
TextHRDMapper::~TextHRDMapper()
{
  for (auto rdef = regionDefines.begin(); rdef != regionDefines.end(); ++rdef) {
    const TextRegion* rd = TextRegion::cast(rdef->second);
    delete rd->start_text;
    delete rd->end_text;
    delete rd->start_back;
    delete rd->end_back;
    delete rd;
  }
}

/** Loads region definitions from HRD file.
    Multiple files could be loaded.
*/
void TextHRDMapper::loadRegionMappings(XmlInputSource* is)
{
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.parse(*is->getInputSource());
  if (error_handler.getSawErrors()) {
    throw Exception(CString("Error loading HRD file"));
  }
  xercesc::DOMDocument* hrdbase = xml_parser.getDocument();
  xercesc::DOMElement* hbase = hrdbase->getDocumentElement();

  if (hbase == nullptr || !xercesc::XMLString::equals(hbase->getNodeName(), hrdTagHrd)) {
    throw Exception(CString("Error loading HRD file"));
  }
  for (xercesc::DOMNode* curel = hbase->getFirstChild(); curel; curel = curel->getNextSibling()) {
    if (curel->getNodeType() == xercesc::DOMNode::ELEMENT_NODE && xercesc::XMLString::equals(curel->getNodeName(), hrdTagAssign)) {
      xercesc::DOMElement* subelem = static_cast<xercesc::DOMElement*>(curel);
      const XMLCh* xname = subelem->getAttribute(hrdAssignAttrName);
      if (*xname == '\0') {
        continue;
      }

      const String* name = new CString(xname);
      auto tp = regionDefines.find(name);
      if (tp != regionDefines.end()) {
        const TextRegion* rd = TextRegion::cast(tp->second);
        delete rd->start_text;
        delete rd->end_text;
        delete rd->start_back;
        delete rd->end_back;
        delete rd;
      }
      const String* stext = nullptr;
      const String* etext = nullptr;
      const String* sback = nullptr;
      const String* eback = nullptr;
      const XMLCh* sval;
      sval = subelem->getAttribute(hrdAssignAttrSText);
      if (*sval != '\0') {
        stext = new SString(CString(sval));
      }
      sval = subelem->getAttribute(hrdAssignAttrEText);
      if (*sval != '\0') {
        etext = new SString(CString(sval));
      }
      sval = subelem->getAttribute(hrdAssignAttrSBack);
      if (*sval != '\0') {
        sback = new SString(CString(sval));
      }
      sval = subelem->getAttribute(hrdAssignAttrEBack);
      if (*sval != '\0') {
        eback = new SString(CString(sval));
      }

      RegionDefine* rdef = new TextRegion(stext, etext, sback, eback);
      std::pair<SString, RegionDefine*> pp(name, rdef);
      regionDefines.emplace(pp);
      delete name;
    }
  }
}

/** Writes all currently loaded region definitions into
    XML file. Note, that this method writes all loaded
    defines from all loaded HRD files.
*/
void TextHRDMapper::saveRegionMappings(Writer* writer) const
{
  writer->write(CString("<?xml version=\"1.0\"?>\n\
<!DOCTYPE hrd SYSTEM \"../hrd.dtd\">\n\n\
<hrd>\n"));
  for (auto it = regionDefines.begin(); it != regionDefines.end(); ++it) {
    const TextRegion* rdef = TextRegion::cast(it->second);
    writer->write(SString("  <define name='") + it->first + "'");
    if (rdef->start_text != nullptr) {
      writer->write(SString(" start_text='") + rdef->start_text + "'");
    }
    if (rdef->end_text != nullptr) {
      writer->write(SString(" end_text='") + rdef->end_text + "'");
    }
    if (rdef->start_back != nullptr) {
      writer->write(SString(" start_back='") + rdef->start_back + "'");
    }
    if (rdef->end_back != nullptr) {
      writer->write(SString(" end_back='") + rdef->end_back + "'");
    }
    writer->write(CString("/>\n"));
  }
  writer->write(CString("\n</hrd>\n"));
}

/** Adds or replaces region definition */
void TextHRDMapper::setRegionDefine(const String &name, const RegionDefine* rd)
{
  const TextRegion* rd_new = TextRegion::cast(rd);
  const String* stext = nullptr;
  const String* etext = nullptr;
  const String* sback = nullptr;
  const String* eback = nullptr;
  if (rd_new->start_text != nullptr) {
    stext = new SString(rd_new->start_text);
  }
  if (rd_new->end_text != nullptr) {
    etext = new SString(rd_new->end_text);
  }
  if (rd_new->start_back != nullptr) {
    sback = new SString(rd_new->start_back);
  }
  if (rd_new->end_back != nullptr) {
    eback = new SString(rd_new->end_back);
  }

  auto rd_old = regionDefines.find(name);
  if (rd_old != regionDefines.end()) {
    const TextRegion* rdef = TextRegion::cast(rd_old->second);
    delete rdef->start_text;
    delete rdef->end_text;
    delete rdef->start_back;
    delete rdef->end_back;
    delete rdef;
  }

  RegionDefine* new_region = new TextRegion(stext, etext, sback, eback);
  std::pair<SString, RegionDefine*> p(name, new_region);
  regionDefines.emplace(p);

  // Searches and replaces old region references
  for (size_t idx = 0; idx < regionDefinesVector.size(); idx++)
    if (regionDefinesVector.at(idx) == rd_old->second) {
      regionDefinesVector.at(idx) = new_region;
      break;
    }
}



