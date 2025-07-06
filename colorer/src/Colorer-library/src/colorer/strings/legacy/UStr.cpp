#include "colorer/strings/legacy/UStr.h"
#include <string>

std::string UStr::to_stdstr(const UnicodeString* str)
{
  std::string out_str(str->getChars());
  return out_str;
}

std::string UStr::to_stdstr(const uUnicodeString& str)
{
  return to_stdstr(str.get());
}

std::wstring UStr::to_stdwstr(const UnicodeString* str)
{
  std::wstring out_string(str->getWChars());
  return out_string;
}

std::wstring UStr::to_stdwstr(const UnicodeString& str)
{
  return to_stdwstr(&str);
}

std::unique_ptr<CharacterClass> UStr::createCharClass(const UnicodeString& ccs, int pos, int* retPos, bool ignore_case)
{
  return CharacterClass::createCharClass(ccs, pos, retPos, ignore_case);
}

int8_t UStr::caseCompare(const UnicodeString& str1, const UnicodeString& str2)
{
  return str1.caseCompare(str2);
}

UnicodeString UStr::to_unistr(int number)
{
  return {number};
}

UnicodeString UStr::to_unistr(const std::string& str)
{
  return {str.c_str(), static_cast<int32_t>(str.length()), Encodings::ENC_UTF8};
}

UnicodeString UStr::to_unistr(const std::wstring& str)
{
  return {str.c_str()};
}

bool UStr::HexToUInt(const UnicodeString& str_hex, unsigned int* result)
{
  UnicodeString s;
  if (str_hex[0] == '#') {
    s = UnicodeString(str_hex, 1);
  }
  else {
    s = str_hex;
  }

  try {
    *result = std::stoul(UStr::to_stdstr(&s), nullptr, 16);
    return true;
  } catch (std::exception& e) {
    COLORER_LOG_ERROR("Can`t convert % to int. %", str_hex, e.what());
    return false;
  }
}

int32_t UStr::indexOfIgnoreCase(const UnicodeString& str1, const UnicodeString& str2, int32_t pos)
{
  return str1.indexOfIgnoreCase(str2, pos);
}
