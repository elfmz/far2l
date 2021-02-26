#include "FarHrcSettings.h"
#include <utils.h>
#include <KeyFileHelper.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <colorer/parsers/ParserFactoryException.h>
#include <xercesc/parsers/XercesDOMParser.hpp>

const char* FarCatalogXml = "/base/catalog.xml";
const char* FarProfileXml = "/plug/hrcsettings.xml";

XERCES_CPP_NAMESPACE_USE

FarHrcSettings::FarHrcSettings(ParserFactory *_parserFactory)
  :
  parserFactory(_parserFactory),
  profileIni(InMyConfig("plugins/colorer/HrcSettings.ini"))
{
}

void FarHrcSettings::readProfile()
{
  StringBuffer *path = GetConfigPath(SString(FarProfileXml));
  readXML(path, false);
  delete path;
}

//
// Method is borrowed from FarColorer.
//
void FarHrcSettings::readXML(String *file, bool userValue)
{
  XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  uXmlInputSource config = XmlInputSource::newInstance(file->getW2Chars(),
                                                       static_cast<XMLCh*>(nullptr));
  xml_parser.parse(*(config->getInputSource()));
  if (error_handler.getSawErrors()) {
    throw ParserFactoryException(SString("Error reading hrcsettings.xml."));
  }
  DOMDocument* catalog = xml_parser.getDocument();
  DOMElement* elem = catalog->getDocumentElement();

  const XMLCh* tagPrototype = (const XMLCh*)u"prototype";
  const XMLCh* tagHrcSettings = (const XMLCh*)u"hrc-settings";

  if (elem == nullptr || !XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    throw FarHrcSettingsException(SString("main '<hrc-settings>' block not found"));
  }
  for (DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
      DOMElement* subelem = static_cast<DOMElement*>(node);
      if (XMLString::equals(subelem->getNodeName(), tagPrototype)) {
        UpdatePrototype(subelem, userValue);
      }
    }
  }
}

//
// Method is borrowed from FarColorer.
//
void FarHrcSettings::UpdatePrototype(DOMElement* elem, bool userValue)
{
  const XMLCh* tagProtoAttrParamName = (const XMLCh*)u"name";
  const XMLCh* tagParam = (const XMLCh*)u"param";
  const XMLCh* tagParamAttrParamName = (const XMLCh*)u"name";
  const XMLCh* tagParamAttrParamValue = (const XMLCh*)u"value";
  const XMLCh* tagParamAttrParamDescription = (const XMLCh*)u"description";
  const XMLCh* typeName = elem->getAttribute(tagProtoAttrParamName);
  if (!XMLString::stringLen(typeName)) {
    return;
  }
  HRCParser* hrcParser = parserFactory->getHRCParser();
  SString typenamed(typeName);
  FileTypeImpl* type = static_cast<FileTypeImpl*>(hrcParser->getFileType(&typenamed));
  if (type == nullptr) {
    return;
  }

  for (DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
      DOMElement* subelem = static_cast<DOMElement*>(node);
      if (XMLString::equals(subelem->getNodeName(), tagParam)) {
        const XMLCh* name = subelem->getAttribute(tagParamAttrParamName);
        const XMLCh* value = subelem->getAttribute(tagParamAttrParamValue);
        const XMLCh* descr = subelem->getAttribute(tagParamAttrParamDescription);

        if (*name == '\0' || *value == '\0') {
          continue;
        }

        if (type->getParamValue(SString(name)) == nullptr) {
          SString tmp(name);
          type->addParam(&tmp);
        }
        if (descr != nullptr) {
          SString tmp(descr);
          type->setParamDescription(SString(name), &tmp);
        }
        if (userValue) {
          SString tmp(value);
          type->setParamValue(SString(name), &tmp);
        } else {
          SString tmp(value);
          delete type->getParamDefaultValue(SString(name));
          type->setParamDefaultValue(SString(name), &tmp);
        }
      }
    }
  }
}

void FarHrcSettings::readUserProfile()
{
  KeyFileReadHelper kfh(profileIni);
  const auto &sections = kfh.EnumSections();

  HRCParser *hrcParser = parserFactory->getHRCParser();
  for (const auto &s : sections)
  {
    StringBuffer ssk(s.c_str());
    FileTypeImpl *type = (FileTypeImpl *)hrcParser->getFileType(&ssk);
    if (type == nullptr){
      continue;
    }
    const auto *Values = kfh.GetSectionValues(s);
    if (!Values) {
      continue;
    }
    const auto &Names = Values->EnumKeys();
    for (const auto &n : Names) {
      StringBuffer ssn(n.c_str());
      if (type->getParamValue(ssn)==nullptr){
        type->addParam(&ssn);
      }
      delete type->getParamUserValue(ssn);
      const auto &v = Values->GetString(n, L"");
      StringBuffer ssv(v.c_str());
      type->setParamValue(ssn, &ssv);
    }
  }
}

void FarHrcSettings::writeUserProfile()
{
  KeyFileHelper kfh(profileIni);
  HRCParser *hrcParser = parserFactory->getHRCParser();

  // enum all FileTypes
  for (int idx = 0; ; ++idx) {
    FileTypeImpl *type = (FileTypeImpl *)hrcParser->enumerateFileTypes(idx);
    if (!type) {
      break;
    }

    kfh.RemoveSection(type->getName()->getChars());
    if (type->getParamCount() && type->getParamUserValueCount()){// params>0 and user values >0
      std::vector<SString> params = type->enumParams();
      // enum all params
      for (const auto &p: params) {
        const String* v = type->getParamUserValue(p);
        if (v != nullptr) {
          StringBuffer tmp(p);
          kfh.PutString(type->getName()->getChars(), tmp.getChars(), v->getChars());
        }
      }
    }
  }
}
