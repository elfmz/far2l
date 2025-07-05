#include "FarHrcSettings.h"
#include <KeyFileHelper.h>
#include <colorer/base/XmlTagDefs.h>
#include <colorer/xml/XmlReader.h>
#include <utils.h>
#include "colorer/parsers/CatalogParser.h"

FarHrcSettings::FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory* _parserFactory)
    : farEditorSet(_farEditorSet),
      parserFactory(_parserFactory),
      profileIni(InMyConfig("plugins/colorer/HrcSettings.ini"))
{
}

void FarHrcSettings::readProfile()
{
  UnicodeString* path = GetConfigPath(FarProfileXml);
  parserFactory->loadHrcSettings(path,false);
  delete path;
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
