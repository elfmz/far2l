#include "colorer/strings/icu/UStr.h"
#include "colorer/strings/icu/UnicodeTools.h"

UnicodeString UStr::to_unistr(const int number)
{
  return {std::to_string(number).c_str()};
}

std::string UStr::to_stdstr(const UnicodeString* str)
{
  std::string out_str;
  if (str) {
    str->toUTF8String(out_str);
  }
  return out_str;
}

std::string UStr::to_stdstr(const uUnicodeString& str)
{
  return to_stdstr(str.get());
}

#ifdef _WINDOWS
// wchar_t and UChar are the same size

std::wstring UStr::to_stdwstr(const uUnicodeString& str)
{
  return to_stdwstr(str.get());
}

std::wstring UStr::to_stdwstr(const UnicodeString& str)
{
  return to_stdwstr(&str);
}

std::wstring UStr::to_stdwstr(const UnicodeString* str)
{
  std::wstring out_string;
  if (str) {
    auto len = str->length();
    auto out_s = std::make_unique<wchar_t[]>(len + 1);
    str->extract(0, len, out_s.get());
    out_s[len] = 0;

    out_string.assign(out_s.get());
  }
  return out_string;
}
#endif

std::unique_ptr<CharacterClass> UStr::createCharClass(const UnicodeString& ccs, int pos, int* retPos, bool ignore_case)
{
  if (ccs[pos] != '[') {
    return nullptr;
  }

  auto cc = std::make_unique<icu::UnicodeSet>();
  icu::UnicodeSet cc_temp;
  bool inverse = false;
  UChar prev_char = BAD_WCHAR;
  UErrorCode ec = U_ZERO_ERROR;

  pos++;
  if (ccs[pos] == '^') {
    inverse = true;
    pos++;
  }

  for (; pos < ccs.length(); pos++) {
    if (ccs[pos] == ']') {
      if (retPos != nullptr) {
        *retPos = pos;
      }
      if (inverse) {
        cc->complement();
      }
      return cc;
    }
    if (ccs[pos] == '{') {
      auto categ = UnicodeTools::getCurlyContent(ccs, pos);
      if (categ == nullptr) {
        return nullptr;
      }
      /*if (*categ == "ALL") cc->add(icu::UnicodeSet::MIN_VALUE, icu::UnicodeSet::MAX_VALUE);
      else if (*categ == "ASSIGNED") cc->addCategory("");
      else if (*categ == "UNASSIGNED") {
        cc_temp.clear();
        cc_temp.addCategory("");
        cc->fill();
        cc->clearClass(cc_temp);
      } else */
      if (categ->length()) {
        cc->addAll(icu::UnicodeSet("\\p{" + *categ + "}", ec));
      }
      pos += categ->length() + 1;
      prev_char = BAD_WCHAR;
      continue;
    }
    if (ccs[pos] == '\\' && pos + 1 < ccs.length()) {
      int retEnd;
      prev_char = BAD_WCHAR;
      switch (ccs[pos + 1]) {
        case 'd':
          cc->addAll(icu::UnicodeSet("[:Nd:]", ec));
          break;
        case 'D':
          cc->addAll(icu::UnicodeSet(icu::UnicodeSet::MIN_VALUE, icu::UnicodeSet::MAX_VALUE)
                         .removeAll(icu::UnicodeSet("\\p{Nd}", ec)));
          break;
        case 'w':
          cc->addAll(icu::UnicodeSet("[:L:]", ec)).addAll(icu::UnicodeSet("\\p{Nd}", ec)).add("_");
          break;
        case 'W':
          cc->addAll(icu::UnicodeSet(icu::UnicodeSet::MIN_VALUE, icu::UnicodeSet::MAX_VALUE)
                         .removeAll(icu::UnicodeSet("\\p{Nd}", ec))
                         .removeAll(icu::UnicodeSet("\\p{L}", ec)))
              .remove("_");
          break;
        case 's':
          cc->addAll(icu::UnicodeSet("[:Z:]", ec)).addAll("\t\n\r\f");
          break;
        case 'S':
          cc->addAll(icu::UnicodeSet(icu::UnicodeSet::MIN_VALUE, icu::UnicodeSet::MAX_VALUE)
                         .removeAll(icu::UnicodeSet("[:Z:]", ec)))
              .removeAll("\t\n\r\f");
          break;
        case 'l':
          cc->addAll(icu::UnicodeSet("[:Ll:]", ec));
          if (ignore_case) {
            cc->addAll(icu::UnicodeSet("[:Lu:]", ec));
          }
          break;
        case 'u':
          cc->addAll(icu::UnicodeSet("[:Lu:]", ec));
          if (ignore_case) {
            cc->addAll(icu::UnicodeSet("[:Ll:]", ec));
          }
          break;
        default:
          prev_char = UnicodeTools::getEscapedChar(ccs, pos, retEnd);
          if (prev_char == BAD_WCHAR) {
            break;
          }
          cc->add(prev_char);
          if (ignore_case) {
            cc->add(u_tolower(prev_char));
            cc->add(u_toupper(prev_char));
            cc->add(u_totitle(prev_char));
          }
          pos = retEnd - 1;
          break;
      }
      pos++;
      continue;
    }
    // substract -[class]
    if (pos + 1 < ccs.length() && ccs[pos] == '-' && ccs[pos + 1] == '[') {
      int retEnd;
      auto scc = createCharClass(ccs, pos + 1, &retEnd, false);
      if (retEnd == ccs.length() || scc == nullptr) {
        return nullptr;
      }
      cc->removeAll(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    // intersect &&[class]
    if (pos + 2 < ccs.length() && ccs[pos] == '&' && ccs[pos + 1] == '&' && ccs[pos + 2] == '[') {
      int retEnd;
      auto scc = createCharClass(ccs, pos + 2, &retEnd, false);
      if (retEnd == ccs.length() || scc == nullptr) {
        return nullptr;
      }
      cc->retainAll(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    // add [class]
    if (ccs[pos] == '[') {
      int retEnd;
      auto scc = createCharClass(ccs, pos, &retEnd, ignore_case);
      if (scc == nullptr) {
        return nullptr;
      }
      cc->addAll(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    if (ccs[pos] == '-' && prev_char != BAD_WCHAR && pos + 1 < ccs.length() && ccs[pos + 1] != ']') {
      int retEnd;
      UChar nextc = UnicodeTools::getEscapedChar(ccs, pos + 1, retEnd);
      if (nextc == BAD_WCHAR) {
        break;
      }
      cc->add(prev_char, nextc);
      pos = retEnd;
      continue;
    }
    cc->add(ccs[pos]);
    if (ignore_case) {
      cc->add(u_tolower(prev_char));
      cc->add(u_toupper(prev_char));
      cc->add(u_totitle(prev_char));
    }
    prev_char = ccs[pos];
  }

  return nullptr;
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

int8_t UStr::caseCompare(const UnicodeString& str1, const UnicodeString& str2)
{
  return str1.caseCompare(str2, U_FOLD_CASE_DEFAULT);
}

int32_t UStr::indexOfIgnoreCase(const UnicodeString& str1, const UnicodeString& str2, int32_t pos)
{
  auto tmp_str1 = str1;
  auto tmp_str2 = str2;
  return tmp_str1.toUpper().indexOf(tmp_str2.toUpper(), pos);
}

#ifndef COLORER_FEATURE_LIBXML
std::unique_ptr<XMLCh[]> UStr::to_xmlch(const UnicodeString* str)
{
  // XMLCh and UChar are the same size
  std::unique_ptr<XMLCh[]> out_s;
  if (str) {
    auto len = str->length();
    out_s = std::make_unique<XMLCh[]>(len + 1);
    str->extract(0, len, out_s.get());
    out_s[len] = 0;
  }
  return out_s;
}

std::string UStr::to_stdstr(const XMLCh* str)
{
  std::string _string = std::string(xercesc::XMLString::transcode(str));
  return _string;
}
#endif
