#include "FarHrcSettings.h"
#include <KeyFileHelper.h>
#include <colorer/base/XmlTagDefs.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <utils.h>
#include <xercesc/parsers/XercesDOMParser.hpp>
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

  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler err_handler;
  xml_parser.setErrorHandler(&err_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  uXmlInputSource config = XmlInputSource::newInstance(filename);
  xml_parser.parse(*config->getInputSource());
  if (err_handler.getSawErrors()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(*filename));
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();
  const XMLCh* tagHrdSets = catTagHrdSets;
  const XMLCh* tagHrd = catTagHrd;
  if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrdSets)) {
    throw Exception("main '<hrd-sets>' block not found");
  }
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagHrd)) {
        auto hrd = CatalogParser::parseHRDSetsChild(subelem);
        if (hrd)
          parserFactory->addHrd(std::move(hrd));
      }
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
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.setDisableDefaultEntityResolution(true);
  uXmlInputSource config = XmlInputSource::newInstance(file);
  xml_parser.parse(*(config->getInputSource()));
  if (error_handler.getSawErrors()) {
    throw ParserFactoryException("Error reading hrcsettings.xml.");
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();

  const XMLCh* tagPrototype = hrcTagPrototype;
  const XMLCh* tagHrcSettings = (const XMLCh*) u"hrc-settings\0";

  if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    throw FarHrcSettingsException("main '<hrc-settings>' block not found");
  }
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      xercesc::DOMElement* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagPrototype)) {
        UpdatePrototype(subelem);
      }
    }
  }
}

void FarHrcSettings::UpdatePrototype(xercesc::DOMElement* elem)
{
  auto typeName = elem->getAttribute(hrcPrototypeAttrName);
  if (typeName == nullptr) {
    return;
  }
  auto& hrcLibrary = parserFactory->getHrcLibrary();
  UnicodeString typenamed = UnicodeString(typeName);
  auto* type = hrcLibrary.getFileType(&typenamed);
  if (type == nullptr) {
    return;
  }

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr;
       node = node->getNextSibling())
  {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), hrcTagParam)) {
        auto name = subelem->getAttribute(hrcParamAttrName);
        auto value = subelem->getAttribute(hrcParamAttrValue);
        auto descr = subelem->getAttribute(hrcParamAttrDescription);

        if (UStr::isEmpty(name)) {
          continue;
        }

        UnicodeString cname = UnicodeString(name);
        UnicodeString cvalue = UnicodeString(value);
        UnicodeString cdescr = UnicodeString(descr);
        if (type->getParamValue(cname) == nullptr) {
          type->addParam(cname, cvalue);
        }
        else {
          type->setParamDefaultValue(cname, &cvalue);
        }
        if (descr != nullptr) {
          type->setParamDescription(cname, &cdescr);
        }
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
