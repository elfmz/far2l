#include "FarHrcSettings.h"
#include <colorer/xml/XmlParserErrorHandler.h>
#include <colorer/parsers/ParserFactoryException.h>
#include <xercesc/parsers/XercesDOMParser.hpp>

const char* FarCatalogXml = "/base/catalog.xml";
const char* FarProfileXml = "/plug/hrcsettings.xml";

XERCES_CPP_NAMESPACE_USE

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
  wchar_t key[MAX_KEY_LENGTH];
  swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings", Info.RootKey);
  HKEY dwKey = nullptr;

  if (WINPORT(RegOpenKeyEx)( HKEY_CURRENT_USER, key, 0, KEY_READ, &dwKey) == ERROR_SUCCESS ){
    readProfileFromRegistry(dwKey);
  }

  WINPORT(RegCloseKey)(dwKey);
}

void FarHrcSettings::readProfileFromRegistry(HKEY dwKey)
{
  DWORD dwKeyIndex=0;
  wchar_t szNameOfKey[MAX_KEY_LENGTH]; 
  DWORD dwBufferSize=MAX_KEY_LENGTH;

  HRCParser *hrcParser = parserFactory->getHRCParser();

  // enum all the sections in HrcSettings
  while(WINPORT(RegEnumKeyEx)(dwKey, dwKeyIndex++, szNameOfKey, &dwBufferSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
  {
    //check whether we have such a scheme
    StringBuffer ssk(szNameOfKey);
    FileTypeImpl *type = (FileTypeImpl *)hrcParser->getFileType(&ssk);
    if (type== nullptr){
      //restore buffer size value
      dwBufferSize=MAX_KEY_LENGTH;
      continue;
    };

    wchar_t key[MAX_KEY_LENGTH];
    HKEY hkKey;

    swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings/%ls", Info.RootKey,szNameOfKey);

    if (WINPORT(RegOpenKeyEx)( HKEY_CURRENT_USER, key, 0, KEY_READ, &hkKey) == ERROR_SUCCESS ){

      DWORD dwValueIndex=0;
      wchar_t szNameOfValue[MAX_VALUE_NAME]; 
      DWORD dwNameOfValueBufferSize=MAX_VALUE_NAME;
      DWORD dwValueBufferSize;
      DWORD nValueType;
      // enum all params in the section
      while(WINPORT(RegEnumValue)(hkKey, dwValueIndex, szNameOfValue, &dwNameOfValueBufferSize, nullptr, &nValueType, nullptr, &dwValueBufferSize) == ERROR_SUCCESS)
      { 
        if (nValueType==REG_SZ){
          
          wchar_t *szValue= new wchar_t[(dwValueBufferSize / sizeof(wchar_t))+1];
          if (WINPORT(RegQueryValueEx)(hkKey, szNameOfValue, 0, nullptr, (PBYTE)szValue, &dwValueBufferSize) == ERROR_SUCCESS){
            StringBuffer ssn(szNameOfValue);
            if (type->getParamValue(ssn)==nullptr){
              type->addParam(&ssn);
            }
            delete type->getParamUserValue(ssn);
            StringBuffer ssv(szValue);
            type->setParamValue(ssn, &ssv);
          }
          delete [] szValue;
        }

        //restore buffer size value
        dwNameOfValueBufferSize=MAX_VALUE_NAME;
        dwValueIndex++;
      }
    }
    WINPORT(RegCloseKey)(hkKey);

    //restore buffer size value
    dwBufferSize=MAX_KEY_LENGTH;
  }

}

void FarHrcSettings::writeUserProfile()
{
  wchar_t key[MAX_KEY_LENGTH];
  swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings", Info.RootKey);
  HKEY dwKey;

  //create or open key
  if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_ALL_ACCESS,
              nullptr, &dwKey, nullptr) == ERROR_SUCCESS ){
    writeProfileToRegistry();
  }

  WINPORT(RegCloseKey)(dwKey);
}

void FarHrcSettings::writeProfileToRegistry()
{
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = nullptr;

  // enum all FileTypes
  for (int idx = 0; ; idx++){
    type =(FileTypeImpl *) hrcParser->enumerateFileTypes(idx);

    if (!type){
      break;
    }

    wchar_t key[MAX_KEY_LENGTH];
    swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings/%ls", Info.RootKey,type->getName()->getWChars());

    WINPORT(RegDeleteKey)(HKEY_CURRENT_USER,key);
    if (type->getParamCount() && type->getParamUserValueCount()){// params>0 and user values >0
      const String* v = nullptr;
      std::vector<SString> params = type->enumParams();
      // enum all params
      for (const auto &p: params) {
        v=type->getParamUserValue(p);
        if (v!=nullptr){
          HKEY hkKey;

          if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkKey, nullptr) == ERROR_SUCCESS ){
            StringBuffer tmp(p);
            WINPORT(RegSetValueEx)(hkKey, tmp.getWChars(), 0, REG_SZ,
                    (const BYTE*)v->getWChars(),
                    sizeof(wchar_t) * (v->length() + 1));
          }
          WINPORT(RegCloseKey)(hkKey);
        }
      }
    }
  }

}
