#include "FarHrcSettings.h"
#include <KeyFileHelper.h>
#include <utils.h>

FarHrcSettings::FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory* _parserFactory)
    : farEditorSet(_farEditorSet),
      parserFactory(_parserFactory),
      profileIni(InMyConfig("plugins/colorer/HrcSettings.ini"))
{
}

void FarHrcSettings::applySettings(const UnicodeString* catalog_xml, const UnicodeString* user_hrd,
                                   const UnicodeString* user_hrc,
                                   const UnicodeString* user_hrc_settings)
{
  parserFactory->loadCatalog(catalog_xml);
  readSystemHrcSettings();
  parserFactory->loadHrdPath(user_hrd);
  parserFactory->loadHrcPath(user_hrc);
  parserFactory->loadHrcSettings(user_hrc_settings, true);
  readUserProfile();
}

void FarHrcSettings::readSystemHrcSettings()
{
  auto path = GetConfigPath(FarProfileXml);
  parserFactory->loadHrcSettings(path.get(), false);
}

void FarHrcSettings::readUserProfile()
{
  KeyFileReadHelper kfh(profileIni);
  const auto& sections = kfh.EnumSections();

  auto& hrcLibrary = parserFactory->getHrcLibrary();
  auto def_filetype = hrcLibrary.getFileType("default");
  for (const auto& s : sections) {
    UnicodeString ssk(s.c_str());
    FileType* type = hrcLibrary.getFileType(&ssk);
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
