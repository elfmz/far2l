#ifndef COLORER_USTR_H
#define COLORER_USTR_H

#include <filesystem>
#include <xercesc/util/XMLString.hpp>
#include "colorer/Common.h"
#include "colorer/strings/legacy/CharacterClass.h"

class UStr
{
 public:

  [[nodiscard]] static UnicodeString to_unistr(int number);
  [[nodiscard]] static std::string to_stdstr(const XMLCh* str);
  [[nodiscard]] static std::string to_stdstr(const UnicodeString* str);
  [[nodiscard]] static std::string to_stdstr(const uUnicodeString& str);
  [[nodiscard]] static std::wstring to_stdwstr(const UnicodeString* str);
  [[nodiscard]] static std::unique_ptr<XMLCh[]> to_xmlch(const UnicodeString* str);
  [[nodiscard]] static std::filesystem::path to_filepath(const uUnicodeString& str);
  inline static bool isEmpty(const XMLCh* string) { return *string == '\0'; }

  static std::unique_ptr<CharacterClass> createCharClass(const UnicodeString& ccs, int pos, int* retPos, bool ignore_case);

  static int8_t caseCompare(const UnicodeString& str1, const UnicodeString& str2);
  static bool HexToUInt(const UnicodeString& str_hex, unsigned int* result);
};

#endif  // COLORER_USTR_H
