#include"FarHrcSettings.h"

void FarHrcSettings::readProfile()
{
  StringBuffer *path = GetConfigPath(DString(FarProfileXml));
  readXML(path, false);
  delete path;
}

void FarHrcSettings::readXML(String *file, bool userValue)
{
  DocumentBuilder docbuilder;
  InputSource *dfis = InputSource::newInstance(file);
  Document *xmlDocument = docbuilder.parse(dfis);
  Element *types = xmlDocument->getDocumentElement();

  if (*types->getNodeName() != "hrc-settings"){
    docbuilder.free(xmlDocument);
    throw FarHrcSettingsException(DString("main '<hrc-settings>' block not found"));
  }
  for (Node *elem = types->getFirstChild(); elem; elem = elem->getNextSibling()){
    if (*elem->getNodeName() == "prototype"){
      UpdatePrototype((Element*)elem, userValue);
      continue;
    }
  };
  docbuilder.free(xmlDocument);
  delete dfis;
}

void FarHrcSettings::UpdatePrototype(Element *elem, bool userValue)
{
  const String *typeName = elem->getAttribute(DString("name"));
  if (typeName == null){
    return;
  }
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = (FileTypeImpl *)hrcParser->getFileType(typeName);
  if (type== null){
    return;
  };
  for(Node *content = elem->getFirstChild(); content != null; content = content->getNextSibling()){
    if (*content->getNodeName() == "param"){
      const String *name = ((Element*)content)->getAttribute(DString("name"));
      const String *value = ((Element*)content)->getAttribute(DString("value"));
      const String *descr = ((Element*)content)->getAttribute(DString("description"));
      if (name == null || value == null){
        continue;
      };

      if (type->getParamValue(SString(name))==null){
        type->addParam(name);
      }
      if (descr != null){
        type->setParamDescription(SString(name), descr);
      }
      if (userValue){
        delete type->getParamNotDefaultValue(DString(name));
        type->setParamValue(SString(name), value);
      }
      else{
        delete type->getParamDefaultValue(DString(name));
        type->setParamDefaultValue(SString(name), value);
      }
    };
  };
}

void FarHrcSettings::readUserProfile()
{
  wchar_t key[MAX_KEY_LENGTH];
  swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings", Info.RootKey);
  HKEY dwKey = NULL;

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
  while(WINPORT(RegEnumKeyEx)(dwKey, dwKeyIndex++, szNameOfKey, &dwBufferSize, NULL, NULL, NULL,NULL)==  ERROR_SUCCESS)
  {
    //check whether we have such a scheme
	DString  ssk(szNameOfKey);
    FileTypeImpl *type = (FileTypeImpl *)hrcParser->getFileType(&ssk);
    if (type== null){
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
      while(WINPORT(RegEnumValue)(hkKey, dwValueIndex, szNameOfValue, &dwNameOfValueBufferSize, NULL, &nValueType, NULL ,&dwValueBufferSize)==  ERROR_SUCCESS)
      { 
        if (nValueType==REG_SZ){
          
          wchar_t *szValue= new wchar_t[(dwValueBufferSize / sizeof(wchar_t))+1];
          if (WINPORT(RegQueryValueEx)(hkKey, szNameOfValue, 0, NULL, (PBYTE)szValue, &dwValueBufferSize)==  ERROR_SUCCESS){
            if (type->getParamValue(SString(szNameOfValue))==null){
			SString ssn(szNameOfValue);
              type->addParam(&ssn);
            }
            delete type->getParamNotDefaultValue(SString(szNameOfValue));
			SString ssv(szValue);
            type->setParamValue(SString(szNameOfValue), &ssv);
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
  if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_ALL_ACCESS,
            NULL, &dwKey, NULL)==ERROR_SUCCESS ){
    writeProfileToRegistry();
  }

  WINPORT(RegCloseKey)(dwKey);
}

void FarHrcSettings::writeProfileToRegistry()
{
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = NULL;

  // enum all FileTypes
  for (int idx = 0; ; idx++){
    type =(FileTypeImpl *) hrcParser->enumerateFileTypes(idx);

    if (!type){
      break;
    }

    wchar_t key[MAX_KEY_LENGTH];
    swprintf(key,MAX_KEY_LENGTH, L"%ls/colorer/HrcSettings/%ls", Info.RootKey,type->getName()->getWChars());

    WINPORT(RegDeleteKey)(HKEY_CURRENT_USER,key);
    const String *p, *v;
    if (type->getParamCount() && type->getParamNotDefaultValueCount()){// params>0 and user values >0
      // enum all params
      for (int i=0;;i++){
        p=type->enumerateParameters(i);
        if (!p){
          break;
        }
        v=type->getParamNotDefaultValue(*p);
        if (v!=NULL){
          HKEY hkKey;

          if (WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkKey, NULL)==ERROR_SUCCESS ){
              WINPORT(RegSetValueEx)(hkKey, p->getWChars(), 0, REG_SZ, (const BYTE*)v->getWChars(), sizeof(wchar_t)*(v->length()+1));
          }
          WINPORT(RegCloseKey)(hkKey);

        }
      }
    }
  }

}
