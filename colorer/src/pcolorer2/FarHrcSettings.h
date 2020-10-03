#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

#include <colorer/parsers/FileTypeImpl.h>
#include <colorer/HRCParser.h>
#include <colorer/parsers/ParserFactory.h>
#include <colorer/unicode/SString.h>

#include"pcolorer.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 50 // in msdn 16383 , but we have enough 50

extern const char* FarCatalogXml;
extern const char* FarProfileXml;

class FarHrcSettingsException : public Exception{
public:
  FarHrcSettingsException(){};
  FarHrcSettingsException(const String& msg){
    what_str.append(SString("FarHrcSettingsException: ")).append(msg);
  };
};

class FarHrcSettings
{
  friend class FileTypeImpl;
public:
  FarHrcSettings(ParserFactory *_parserFactory){parserFactory=_parserFactory;}
  void readXML(String *file, bool userValue);
  void readProfile();
  void readUserProfile();
  void writeUserProfile();

private:
  void UpdatePrototype(xercesc::DOMElement* elem, bool userValue);
  void readProfileFromRegistry(HKEY dwKey);
  void writeProfileToRegistry();

  ParserFactory *parserFactory;

};


#endif
