#pragma once

#include <Classes.hpp>

struct TTranslation
{
  Word Language, CharSet;
};

// Return pointer to file version info block
void * CreateFileInfo(const UnicodeString & AFileName);

// Free file version info block memory
void FreeFileInfo(void * FileInfo);

// Return pointer to fixed file version info
TVSFixedFileInfo *GetFixedFileInfo(void * FileInfo);

// Return number of available file version info translations
uint32_t GetTranslationCount(void * FileInfo);

// Return i-th translation in the file version info translation list
TTranslation GetTranslation(void * FileInfo, intptr_t I);

// Return the name of the specified language
UnicodeString GetLanguage(Word Language);

// Return the value of the specified file version info string using the
// specified translation
UnicodeString GetFileInfoString(void * FileInfo,
  TTranslation Translation, const UnicodeString & StringName, bool AllowEmpty);

intptr_t CalculateCompoundVersion(intptr_t MajorVer,
  intptr_t MinorVer, intptr_t Release, intptr_t Build);

intptr_t StrToCompoundVersion(const UnicodeString & AStr);
