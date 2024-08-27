#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

#include <colorer/ParserFactory.h>
#include <string>
#include "FarEditorSet.h"
#include "colorer/xml/XMLNode.h"

const char FarCatalogXml[] = "/base/catalog.xml";
const char FarProfileXml[] = "/plug/hrcsettings.xml";

class FarHrcSettingsException : public Exception
{
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
  FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory* _parserFactory);
  void readXML(UnicodeString* file);
  void readProfile();
  void readUserProfile(const FileType* def_filetype = nullptr);
  void writeUserProfile();
  void loadUserHrc(const UnicodeString* filename);
  void loadUserHrd(const UnicodeString* filename);

 private:
  void UpdatePrototype(const XMLNode& elem);

  FarEditorSet* farEditorSet;
  ParserFactory* parserFactory;
  std::string profileIni;
};

#endif
