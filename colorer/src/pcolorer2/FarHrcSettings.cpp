#include "FarHrcSettings.h"
#include <KeyFileHelper.h>
#include <colorer/base/XmlTagDefs.h>
#include <colorer/xml/XmlReader.h>
#include <utils.h>
#include "colorer/parsers/CatalogParser.h"

void FarHrcSettings::loadUserHrc(const UnicodeString* filename)
{
  if (filename && !filename->isEmpty()) {
    parserFactory->loadHrcPath(*filename);
  }
}

void FarHrcSettings::loadUserHrd(const UnicodeString* filename)
{
  if (!filename || filename->isEmpty()) {
    return;
  }

  XmlInputSource config(*filename);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(*filename));
  }

  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name !=  catTagHrdSets) {
    throw Exception("main '<hrd-sets>' block not found");
  }
  for (const auto& node : nodes.begin()->children) {
    if (node.name == catTagHrd) {
      auto hrd = CatalogParser::parseHRDSetsChild(node);
      if (hrd)
        parserFactory->addHrd(std::move(hrd));
    }
  }
}

FarHrcSettings::FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory* _parserFactory)
    : farEditorSet(_farEditorSet),
      parserFactory(_parserFactory),
      profileIni(InMyConfig("plugins/colorer/HrcSettings.ini"))
{
}

void FarHrcSettings::readProfile()
{
  UnicodeString* path = GetConfigPath(FarProfileXml);
  readXML(path);
  delete path;
}

void FarHrcSettings::readXML(UnicodeString* file)
{
  XmlInputSource config(*file);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException("Error reading hrcsettings.xml.");
  }

  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name !=  u"hrc-settings") {
    throw FarHrcSettingsException("main '<hrc-settings>' block not found");
  }

  for (const auto& node : nodes.begin()->children) {
    if (node.name == hrcTagPrototype) {
      UpdatePrototype(node);
    }
  }
}

void FarHrcSettings::UpdatePrototype(const XMLNode& elem)
{
  const auto& typeName = elem.getAttrValue(hrcPrototypeAttrName);
  if (typeName.isEmpty()) {
    return;
  }
  auto& hrcLibrary = parserFactory->getHrcLibrary();
  auto* type = hrcLibrary.getFileType(typeName);
  if (type == nullptr) {
    return;
  }

  for (const auto& node : elem.children) {
    if (node.name == hrcTagParam) {
      const auto& name = node.getAttrValue(hrcParamAttrName);
      const auto& value = node.getAttrValue(hrcParamAttrValue);
      const auto& descr = node.getAttrValue(hrcParamAttrDescription);

      if (name.isEmpty()) {
        continue;
      }

      if (type->getParamValue(name) == nullptr) {
        type->addParam(name, value);
      }
      else {
        type->setParamDefaultValue(name, &value);
      }
      if (descr != nullptr) {
        type->setParamDescription(name, &descr);
      }
    }
  }
}

void FarHrcSettings::readUserProfile(const FileType* def_filetype)
{
  KeyFileReadHelper kfh(profileIni);
  const auto& sections = kfh.EnumSections();

  auto& hrcParser = parserFactory->getHrcLibrary();
  for (const auto& s : sections) {
    UnicodeString ssk(s.c_str());
    FileType* type = hrcParser.getFileType(&ssk);
    if (type == nullptr) {
      continue;
    }
    const auto* Values = kfh.GetSectionValues(s);
    if (!Values) {
      continue;
    }
    const auto& Names = Values->EnumKeys();
    for (const auto& n : Names) {
      UnicodeString ssn(n.c_str());
      const auto& v = Values->GetString(n, L"");
      UnicodeString ssv(v.c_str());
      farEditorSet->addParamAndValue(type, ssn, ssv, def_filetype);
    }
  }
}

void FarHrcSettings::writeUserProfile()
{
  KeyFileHelper kfh(profileIni);
  auto& hrcParser = parserFactory->getHrcLibrary();

  // enum all FileTypes
  for (int idx = 0;; ++idx) {
    FileType* type = hrcParser.enumerateFileTypes(idx);
    if (!type) {
      break;
    }

    kfh.RemoveSection(type->getName().getChars());
    if (type->getParamCount()) {  // params>0 and user values >0
      std::vector<UnicodeString> params = type->enumParams();
      // enum all params
      for (const auto& p : params) {
        const UnicodeString* v = type->getParamUserValue(p);
        if (v != nullptr) {
          UnicodeString tmp(p);
          kfh.SetString(type->getName().getChars(), tmp.getChars(), v->getChars());
        }
      }
    }
  }
}
