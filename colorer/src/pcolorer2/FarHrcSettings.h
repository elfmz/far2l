#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

#include <colorer/parsers/FileTypeImpl.h>
#include <colorer/HrcLibrary.h>
#include <colorer/ParserFactory.h>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include "pcolorer.h"
#include "FarEditorSet.h"
#include <string>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 50 // in msdn 16383 , but we have enough 50

extern const char* FarCatalogXml;
extern const char* FarProfileXml;

class FarHrcSettingsException : public Exception{
public:
  explicit FarHrcSettingsException(const UnicodeString& msg) noexcept
  : Exception("[FarHrcSettingsException] " + msg)
  {
  }
};

class FarHrcSettings
{
  friend class FileTypeImpl;
public:
  FarHrcSettings(FarEditorSet *_farEditorSet, ParserFactory *_parserFactory);
  void readXML(UnicodeString *file, bool userValue);
  void readProfile();
  void readUserProfile();
  void writeUserProfile();

private:
  void UpdatePrototype(xercesc::DOMElement* elem, bool userValue);

  FarEditorSet* farEditorSet;
  ParserFactory *parserFactory;
  std::string profileIni;

};


#endif
