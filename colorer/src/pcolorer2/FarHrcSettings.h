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
  void readProfile();
  void readUserProfile(const FileType* def_filetype = nullptr);
  void writeUserProfile();

 private:
  FarEditorSet* farEditorSet;
  ParserFactory* parserFactory;
  std::string profileIni;
};

#endif
