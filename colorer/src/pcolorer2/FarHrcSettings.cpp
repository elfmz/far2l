#include "FarHrcSettings.h"
#include <utils.h>
#include <KeyFileHelper.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

const char* FarCatalogXml = "/base/catalog.xml";
const char* FarProfileXml = "/plug/hrcsettings.xml";

XERCES_CPP_NAMESPACE_USE

FarHrcSettings::FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory *_parserFactory)
  :
  farEditorSet(_farEditorSet),
  parserFactory(_parserFactory),
  profileIni(InMyConfig("plugins/colorer/HrcSettings.ini"))
{
}

void FarHrcSettings::readProfile()
{
  UnicodeString *path = GetConfigPath(UnicodeString(FarProfileXml));
  readXML(path, false);
  delete path;
}

//
// Method is borrowed from FarColorer.
//
void FarHrcSettings::readXML(UnicodeString *file, bool userValue)
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
    throw ParserFactoryException("Error reading hrcsettings.xml.");
  }
  DOMDocument* catalog = xml_parser.getDocument();
  DOMElement* elem = catalog->getDocumentElement();

  const XMLCh* tagPrototype = (const XMLCh*)u"prototype";
  const XMLCh* tagHrcSettings = (const XMLCh*)u"hrc-settings";

  if (elem == nullptr || !XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    throw FarHrcSettingsException("main '<hrc-settings>' block not found");
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
  auto& hrcParser = parserFactory->getHrcLibrary();
  UnicodeString typenamed(typeName);
  FileType* type = hrcParser.getFileType(&typenamed);
  if (type == nullptr) {
    return;
  }

  for (DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
      DOMElement* subelem = static_cast<DOMElement*>(node);
      if (XMLString::equals(subelem->getNodeName(), tagParam)) {
        auto name = subelem->getAttribute(tagParamAttrParamName);
        auto value = subelem->getAttribute(tagParamAttrParamValue);
        auto descr = subelem->getAttribute(tagParamAttrParamDescription);

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

void FarHrcSettings::readUserProfile()
{
  KeyFileReadHelper kfh(profileIni);
  const auto &sections = kfh.EnumSections();

  auto& hrcParser = parserFactory->getHrcLibrary();
  for (const auto &s : sections)
  {
    UnicodeString ssk(s.c_str());
    FileType *type = hrcParser.getFileType(&ssk);
    if (type == nullptr){
      continue;
    }
    const auto *Values = kfh.GetSectionValues(s);
    if (!Values) {
      continue;
    }
    const auto &Names = Values->EnumKeys();
    for (const auto &n : Names) {
      UnicodeString ssn(n.c_str());
      const auto &v = Values->GetString(n, L"");
      UnicodeString ssv(v.c_str());
      farEditorSet->addParamAndValue(type, ssn, ssv);
    }
  }
}

void FarHrcSettings::writeUserProfile()
{
  KeyFileHelper kfh(profileIni);
  auto& hrcParser = parserFactory->getHrcLibrary();

  // enum all FileTypes
  for (int idx = 0; ; ++idx) {
    FileType *type = hrcParser.enumerateFileTypes(idx);
    if (!type) {
      break;
    }

    kfh.RemoveSection(type->getName().getChars());
    if (type->getParamCount()){// params>0 and user values >0
      std::vector<UnicodeString> params = type->enumParams();
      // enum all params
      for (const auto &p: params) {
        const UnicodeString* v = type->getParamUserValue(p);
        if (v != nullptr) {
          UnicodeString tmp(p);
          kfh.SetString(type->getName().getChars(), tmp.getChars(), v->getChars());
        }
      }
    }
  }
}
