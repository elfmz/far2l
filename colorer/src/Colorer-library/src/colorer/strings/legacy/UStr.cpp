#include "colorer/strings/legacy/UStr.h"
#include <string>

#ifndef COLORER_FEATURE_LIBXML
std::string UStr::to_stdstr(const XMLCh* str)
{
  std::string _string = std::string(xercesc::XMLString::transcode(str));
  return _string;
}

std::unique_ptr<XMLCh[]> UStr::to_xmlch(const UnicodeString* str)
{
  // XMLCh and UChar are the same size
  std::unique_ptr<XMLCh[]> out_s;
  if (str) {
    auto len = str->length();
    out_s = std::make_unique<XMLCh[]>(len + 1);
    memcpy(out_s.get(),str->getW2Chars(),len*2);
    out_s[len] = 0;
  }
  return out_s;
}

#endif

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
  std::wstring out_string (str->getWChars());
  return out_string;
}

std::wstring UStr::to_stdwstr(const UnicodeString& str)
{
  return to_stdwstr(&str);
}

std::unique_ptr<CharacterClass> UStr::createCharClass(const UnicodeString& ccs, int pos,
                                                      int* retPos, bool ignore_case)
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
    logger->error("Can`t convert {0} to int. {1}", str_hex, e.what());
    return false;
  }
}

int32_t UStr::indexOfIgnoreCase(const UnicodeString& str1, const UnicodeString& str2, int32_t pos)
{
  return str1.indexOfIgnoreCase(str2, pos);
}
