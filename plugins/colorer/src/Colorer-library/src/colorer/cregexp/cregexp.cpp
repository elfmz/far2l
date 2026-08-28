#include "colorer/cregexp/cregexp.h"

std::vector<StackElem> CRegExp::RegExpStack;


/////////////////////////////////////////////////////////////////////////////


void SMatches::topseSanitize(int cur)
{
    while (topse < cur) {
      ++topse;
      s[topse] = -1;
      e[topse] = -1;
    }
}

void SMatches::topnseSanitize(int cur)
{
    while (topnse < cur) {
      ++topnse;
      ns[topnse] = -1;
      ne[topnse] = -1;
    }
}


SRegInfo::SRegInfo()
{
  un.param = nullptr;
}
SRegInfo::~SRegInfo()
{
  delete next;
  switch (op) {
    case EOps::ReEnum:
      delete un.charclass;
      break;
    case EOps::ReWord:
      delete un.word;
      break;
    case EOps::ReBrackets:
    case EOps::ReNamedBrackets:
    case EOps::ReOr:
    case EOps::ReAhead:
    case EOps::ReNAhead:
    case EOps::ReBehind:
    case EOps::ReNBehind:
    case EOps::ReRangeN:
    case EOps::ReRangeNM:
    case EOps::ReNGRangeN:
    case EOps::ReNGRangeNM:
      delete un.param;
      break;
    default:
      break;
  }
}

////////////////////////////////////////////////////////////////////////////
// CRegExp class
void CRegExp::init()
{
  tree_root = nullptr;
  positionMoves = false;
  error = EError::EERROR;
  firstNode = nullptr;
  firstCharMask = {};
  firstCharMaskUseful = false;
  cMatch = 0;
  global_pattern = nullptr;
#ifdef COLORERMODE
  backRE = nullptr;
  backStr = nullptr;
  backTrace = nullptr;
  usesBackTrace = false;
#endif
  cnMatch = 0;
  count_elem = 0;
  parseSteps = 0;
  stepBudgetExceeded = false;
}
CRegExp::CRegExp()
{
  init();
}
CRegExp::CRegExp(const UnicodeString* text)
{
  init();
  if (text)
    setRE(text);
}
CRegExp::~CRegExp()
{
  delete tree_root;
  for (int bp = 0; bp < cnMatch; bp++) delete brnames[bp];
}

bool CRegExp::matchChars(wchar one, wchar another) const
{
  // сознательное упрощение. правильнее дополнительно сравнивать toUpperCase, т.к. для Turkish I/ı и части Unicode это разное поведение
  return one == another ||
    (ignoreCase && Character::toLowerCase(one) == Character::toLowerCase(another));
}

EError CRegExp::setRELow(const UnicodeString& expr)
{
  auto len = expr.length();
  if (!len)
    return EError::EERROR;

  delete tree_root;
  tree_root = nullptr;
  firstNode = nullptr;
  firstCharMask = {};
  firstCharMaskUseful = false;
  for (int bp = 0; bp < cnMatch; bp++) delete brnames[bp];

  cMatch = 0;
  cnMatch = 0;
  endChange = startChange = false;
#ifdef COLORERMODE
  usesBackTrace = false;
#endif
  int start = 0;
  while (start < len && Character::isWhitespace(expr[start])) start++;
  if (start >= len || expr[start] != '/')
    return EError::ESYNTAX;
  start++;

  bool ok = false;
  ignoreCase = extend = singleLine = multiLine = false;
  for (auto i = len - 1; i >= start && !ok; i--)
    if (expr[i] == '/') {
      for (auto j = i + 1; j < len; j++) {
        if (expr[j] == 'i')
          ignoreCase = true;
        if (expr[j] == 'x')
          extend = true;
        if (expr[j] == 's')
          singleLine = true;
        if (expr[j] == 'm')
          multiLine = true;
      }
      len = i - start;
      ok = true;
    }
  if (!ok)
    return EError::ESYNTAX;

  // making tree structure
  tree_root = new SRegInfo;
  tree_root->op = EOps::ReBrackets;
  tree_root->un.param = new SRegInfo;
  tree_root->un.param->parent = tree_root;
  tree_root->param0 = cMatch++;

  int endPos;
  UnicodeString body(expr, start, len);
  EError err = setStructs(tree_root->un.param, body, 0, len, endPos);
  if (err != EError::EOK)
    return err;
  if (endPos != len)
    return EError::EBRACKETS;
  optimize();
  return EError::EOK;
}

void CRegExp::optimize()
{
  SRegInfo* next = tree_root;
  firstNode = nullptr;
  while (next) {
    if (next->op == EOps::ReBrackets) {
      next = next->un.param;
      continue;
    }
    if (next->op == EOps::ReAhead || next->op == EOps::ReNAhead ||
        next->op == EOps::ReBehind || next->op == EOps::ReNBehind) {
      next = next->next;
      continue;
    }
    if (next->op == EOps::ReMetaSymb) {
      firstNode = next;
      switch (next->un.metaSymbol) {
        case EMetaSymbols::ReSoL:
        case EMetaSymbols::ReEoL:
        case EMetaSymbols::ReWBound:
        case EMetaSymbols::ReNWBound:
        case EMetaSymbols::RePreNW:
#ifdef COLORERMODE
        case EMetaSymbols::ReSoScheme:
        case EMetaSymbols::ReStart:
        case EMetaSymbols::ReEnd:
#endif
          next = next->next;
          continue;
        default:
          break;
      }
      break;
    }
    if (next->op == EOps::ReSymb || next->op == EOps::ReWord || next->op == EOps::ReEnum) {
      firstNode = next;
    }
    break;
  }

  const auto firstChars = analyzeFirstChars(tree_root);
  firstCharMask = firstChars.mask;
  firstCharMaskUseful = !firstChars.nullable &&
    (firstCharMask[0] != ~uint64_t(0) || firstCharMask[1] != ~uint64_t(0));
}

void CRegExp::addFirstChar(FirstChars& result, wchar ch) const
{
  const auto value = static_cast<uint32_t>(ch);
  if (value >= 128) return;
  result.mask[value >> 6] |= uint64_t(1) << (value & 63);
  if (ignoreCase && value >= 'A' && value <= 'Z') {
    const auto lower = value + ('a' - 'A');
    result.mask[lower >> 6] |= uint64_t(1) << (lower & 63);
  }
  else if (ignoreCase && value >= 'a' && value <= 'z') {
    const auto upper = value - ('a' - 'A');
    result.mask[upper >> 6] |= uint64_t(1) << (upper & 63);
  }
}

CRegExp::FirstChars CRegExp::firstCharsForNode(const SRegInfo* re) const
{
  FirstChars result;
  if (!re) {
    result.nullable = true;
    return result;
  }
  switch (re->op) {
    case EOps::ReSymb:
      addFirstChar(result, re->un.symbol);
      break;
    case EOps::ReWord:
      if (re->un.word->length()) addFirstChar(result, (*re->un.word)[0]);
      else result.nullable = true;
      break;
    case EOps::ReEnum:
      for (uint32_t ch = 0; ch < 128; ch++) {
        if (re->un.charclass->contains(static_cast<wchar>(ch))) {
          result.mask[ch >> 6] |= uint64_t(1) << (ch & 63);
        }
      }
      break;
    case EOps::ReMetaSymb:
      switch (re->un.metaSymbol) {
        case EMetaSymbols::ReAnyChr:
          result.mask = {~uint64_t(0), ~uint64_t(0)};
          if (!singleLine) {
            for (const uint32_t ch : {0x0Au, 0x0Bu, 0x0Cu, 0x0Du})
              result.mask[ch >> 6] &= ~(uint64_t(1) << (ch & 63));
          }
          break;
        case EMetaSymbols::ReDigit:
        case EMetaSymbols::ReNDigit:
        case EMetaSymbols::ReWordSymb:
        case EMetaSymbols::ReNWordSymb:
        case EMetaSymbols::ReWSpace:
        case EMetaSymbols::ReNWSpace:
        case EMetaSymbols::ReUCase:
        case EMetaSymbols::ReNUCase:
          for (uint32_t ch = 0; ch < 128; ch++) {
            bool matchesChar = false;
            switch (re->un.metaSymbol) {
              case EMetaSymbols::ReDigit: matchesChar = Character::isDigit(ch); break;
              case EMetaSymbols::ReNDigit: matchesChar = !Character::isDigit(ch); break;
              case EMetaSymbols::ReWordSymb: matchesChar = Character::isLetterOrDigitOrUnderscore(ch); break;
              case EMetaSymbols::ReNWordSymb: matchesChar = !Character::isLetterOrDigitOrUnderscore(ch); break;
              case EMetaSymbols::ReWSpace: matchesChar = Character::isWhitespace(ch); break;
              case EMetaSymbols::ReNWSpace: matchesChar = !Character::isWhitespace(ch); break;
              case EMetaSymbols::ReUCase: matchesChar = Character::isUpperCase(ch); break;
              case EMetaSymbols::ReNUCase: matchesChar = Character::isLowerCase(ch); break;
              default: break;
            }
            if (matchesChar) result.mask[ch >> 6] |= uint64_t(1) << (ch & 63);
          }
          break;
        default:
          result.nullable = true;
          break;
      }
      break;
    case EOps::ReBrackets:
    case EOps::ReNamedBrackets:
      result = analyzeFirstChars(re->un.param);
      break;
    case EOps::ReOr: {
      result = analyzeFirstChars(re->un.param);
      const auto right = analyzeFirstChars(re->next);
      result.mask[0] |= right.mask[0];
      result.mask[1] |= right.mask[1];
      result.nullable = result.nullable || right.nullable;
      break;
    }
    case EOps::ReRangeN:
    case EOps::ReRangeNM:
    case EOps::ReNGRangeN:
    case EOps::ReNGRangeNM:
      result = analyzeFirstChars(re->un.param);
      result.nullable = re->s == 0 || result.nullable;
      break;
    case EOps::ReAhead:
    case EOps::ReNAhead:
    case EOps::ReBehind:
    case EOps::ReNBehind:
    case EOps::ReEmpty:
      result.nullable = true;
      break;
    default:
      result.mask = {~uint64_t(0), ~uint64_t(0)};
      break;
  }
  return result;
}

CRegExp::FirstChars CRegExp::analyzeFirstChars(const SRegInfo* re) const
{
  FirstChars result;
  result.nullable = true;
  for (auto* node = re; node && result.nullable; node = node->next) {
    const auto current = firstCharsForNode(node);
    result.mask[0] |= current.mask[0];
    result.mask[1] |= current.mask[1];
    result.nullable = current.nullable;
    if (node->op == EOps::ReOr) break;
  }
  return result;
}

EError CRegExp::setStructs(SRegInfo*& re, const UnicodeString& expr, int from, int to, int& retPos)
{
  SRegInfo *next, *temp;

  retPos = 0;
  if (from >= to)
    return EError::EOK;
  retPos = -1;

  next = re;
  for (int i = from; i < to; i++) {
    // simple character
    if (extend && Character::isWhitespace(expr[i]))
      continue;
    // context return
    if (expr[i] == ')') {
      retPos = i - from;
      break;
    }
    // next element
    if (i != from) {
      next->next = new SRegInfo;
      next->next->parent = next->parent;
      next->next->prev = next;
      next = next->next;
    }
    // Escape symbol
    if (expr[i] == '\\') {
      if (i + 1 >= to)
        return EError::ESYNTAX;
      int blen;
      switch (expr[i + 1]) {
        case 'd':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReDigit;
          break;
        case 'D':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReNDigit;
          break;
        case 'w':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReWordSymb;
          break;
        case 'W':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReNWordSymb;
          break;
        case 's':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReWSpace;
          break;
        case 'S':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReNWSpace;
          break;
        case 'u':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReUCase;
          break;
        case 'l':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReNUCase;
          break;
        case 't':
          next->op = EOps::ReSymb;
          next->un.symbol = '\t';
          break;
        case 'n':
          next->op = EOps::ReSymb;
          next->un.symbol = '\n';
          break;
        case 'r':
          next->op = EOps::ReSymb;
          next->un.symbol = '\r';
          break;
        case 'b':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReWBound;
          break;
        case 'B':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReNWBound;
          break;
        case 'c':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::RePreNW;
          break;
#ifdef COLORERMODE
        case 'm':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReStart;
          break;
        case 'M':
          next->op = EOps::ReMetaSymb;
          next->un.metaSymbol = EMetaSymbols::ReEnd;
          break;
        case 'y':
        case 'Y':
          usesBackTrace = true;
          next->op = (expr[i + 1] == 'y' ? EOps::ReBkTrace : EOps::ReBkTraceN);
          if (i + 2 >= to)
            return EError::ESYNTAX;
          next->param0 = UnicodeTools::getHex(expr[i + 2]);
          if (next->param0 != -1) {
            i++;
          }
          else {
            next->op = (expr[i + 1] == 'y' ? EOps::ReBkTraceName : EOps::ReBkTraceNName);
            auto br_name = UnicodeTools::getCurlyContent(expr, i + 2);
            if (br_name == nullptr)
              return EError::ESYNTAX;
            if (!backRE) {
              return EError::EERROR;
            }
            next->param0 = backRE->getBracketNo(br_name.get());
            blen = br_name->length();
            if (next->param0 == -1)
              return EError::ESYNTAX;
            i += blen + 2;
          }
          break;
#endif  // COLORERMODE

        case 'p':  // \p{name}
        {
          if (i + 2 >= to)
            return EError::ESYNTAX;
          next->op = EOps::ReBkBrackName;
          auto br_name = UnicodeTools::getCurlyContent(expr, i + 2);
          if (br_name == nullptr)
            return EError::ESYNTAX;
          blen = br_name->length();
          next->param0 = getBracketNo(br_name.get());
          if (next->param0 == -1)
            return EError::ESYNTAX;
          i += blen + 2;
        } break;
        default:
          next->op = EOps::ReBkBrack;
          next->param0 = UnicodeTools::getHex(expr[i + 1]);
          if (next->param0 < 0 || next->param0 > 9) {
            int retEnd;
            next->op = EOps::ReSymb;
            next->un.symbol = UnicodeTools::getEscapedChar(expr, i, retEnd);
            if (next->un.symbol == BAD_WCHAR)
              return EError::ESYNTAX;
            i = retEnd - 1;
          }
          break;
      }
      i++;
      continue;
    }

    if (expr[i] == '.') {
      next->op = EOps::ReMetaSymb;
      next->un.metaSymbol = EMetaSymbols::ReAnyChr;
      continue;
    }
    if (expr[i] == '^') {
      next->op = EOps::ReMetaSymb;
      next->un.metaSymbol = EMetaSymbols::ReSoL;
      continue;
    }
    if (expr[i] == '$') {
      next->op = EOps::ReMetaSymb;
      next->un.metaSymbol = EMetaSymbols::ReEoL;
      continue;
    }
#ifdef COLORERMODE
    if (expr[i] == '~') {
      next->op = EOps::ReMetaSymb;
      next->un.metaSymbol = EMetaSymbols::ReSoScheme;
      continue;
    }
#endif

    next->un.param = nullptr;
    next->param0 = 0;

    if (to > i + 2) {
      if (expr[i] == '?' && expr[i + 1] == '#' && expr[i + 2] >= '0' && expr[i + 2] <= '9') {
        next->op = EOps::ReBehind;
        next->param0 = UnicodeTools::getHex(expr[i + 2]);
        i += 2;
        continue;
      }
      if (expr[i] == '?' && expr[i + 1] == '~' && expr[i + 2] >= '0' && expr[i + 2] <= '9') {
        next->op = EOps::ReNBehind;
        next->param0 = UnicodeTools::getHex(expr[i + 2]);
        i += 2;
        continue;
      }
    }
    if (to > i + 1) {
      if (expr[i] == '*' && expr[i + 1] == '?') {
        next->op = EOps::ReNGRangeN;
        next->s = 0;
        i++;
        continue;
      }
      if (expr[i] == '+' && expr[i + 1] == '?') {
        next->op = EOps::ReNGRangeN;
        next->s = 1;
        i++;
        continue;
      }
      if (expr[i] == '?' && expr[i + 1] == '=') {
        next->op = EOps::ReAhead;
        i++;
        continue;
      }
      if (expr[i] == '?' && expr[i + 1] == '!') {
        next->op = EOps::ReNAhead;
        i++;
        continue;
      }
      if (expr[i] == '?' && expr[i + 1] == '?') {
        next->op = EOps::ReNGRangeNM;
        next->s = 0;
        next->e = 1;
        i++;
        continue;
      }
    }

    if (expr[i] == '*') {
      next->op = EOps::ReRangeN;
      next->s = 0;
      continue;
    }
    if (expr[i] == '+') {
      next->op = EOps::ReRangeN;
      next->s = 1;
      continue;
    }
    if (expr[i] == '?') {
      next->op = EOps::ReRangeNM;
      next->s = 0;
      next->e = 1;
      continue;
    }
    if (expr[i] == '|') {
      next->op = EOps::ReOr;
      continue;
    }

    // {n,m}
    if (expr[i] == '{') {
      int st = i + 1;
      int en = -1;
      int comma = -1;
      bool nonGreedy = false;
      int j;
      for (j = i; j < to; j++) {
        if (to > j + 1 && expr[j] == '}' && expr[j + 1] == '?') {
          en = j;
          nonGreedy = true;
          j++;
          break;
        }
        if (expr[j] == '}') {
          en = j;
          break;
        }
        if (expr[j] == ',')
          comma = j;
      }
      if (en == -1)
        return EError::EBRACKETS;
      if (comma == -1)
        comma = en;
      next->s = UnicodeTools::getNumber(&expr, st, comma - st);
      if (comma != en)
        next->e = UnicodeTools::getNumber(&expr, comma + 1, en - comma - 1);
      else
        next->e = next->s;
      if (next->s < 0)
        return EError::EOP;
      if (en - comma == 1)
        next->e = -1;
      else if (next->e < next->s)
        return EError::EOP;
      if (next->e == -1)
        next->op = nonGreedy ? EOps::ReNGRangeN : EOps::ReRangeN;
      else
        next->op = nonGreedy ? EOps::ReNGRangeNM : EOps::ReRangeNM;
      i = j;
      continue;
    }
    // ( ... )
    if (expr[i] == '(') {
      // perl-like "uncaptured" brackets
      if (to >= i + 2 && expr[i + 1] == '?' && expr[i + 2] == ':') {
        next->op = EOps::ReNamedBrackets;
        next->param0 = -1;
        i += 3;
      }
      else if (to > i + 2 && expr[i + 1] == '?' && expr[i + 2] == '{') {
        // named bracket
        next->op = EOps::ReNamedBrackets;
        auto s_curly = UnicodeTools::getCurlyContent(expr, i + 2);
        if (s_curly == nullptr)
          return EError::EBRACKETS;
        auto br_name = new UnicodeString(*s_curly);
        auto blen = br_name->length();
        if (blen == 0) {
          next->param0 = -1;
          delete br_name;
        }
        else {
          if (cnMatch < NAMED_MATCHES_NUM) {
            next->param0 = cnMatch;
            brnames[cnMatch] = br_name;
            cnMatch++;
          }
          else {
            // HRC schemes may exceed the slot count; extra groups stay non-capturing
            delete br_name;
            next->param0 = -1;
          }
        }
        i += blen + 4;
      }
      else {
        next->op = EOps::ReBrackets;
        if (cMatch < MATCHES_NUM) {
          next->param0 = cMatch;
          cMatch++;
        }
        else
          next->param0 = -1;
        i += 1;
      }
      next->un.param = new SRegInfo;
      next->un.param->parent = next;
      int endPos;
      EError err = setStructs(next->un.param, expr, i, to, endPos);
      if (to - i - endPos == 0)
        return EError::EBRACKETS;
      if (err != EError::EOK)
        return err;
      i += endPos;
      continue;
    }

    // [] [^]
    if (expr[i] == '[') {
      int endPos;
      auto cc = UStr::createCharClass(expr, i, &endPos, ignoreCase);
      if (cc == nullptr)
        return EError::EENUM;
      next->op = EOps::ReEnum;
      next->un.charclass = cc.release();
      i = endPos;
      continue;
    }
    if (expr[i] == ')' || expr[i] == ']' || expr[i] == '}')
      return EError::EBRACKETS;
    next->op = EOps::ReSymb;
    next->un.symbol = expr[i];
  }

  // operators fixes
  for (next = re; next; next = next->next) {
    // makes words from symbols
    SRegInfo* reword = next;
    SRegInfo* reafterword = next;
    SRegInfo* resymb;
    int wsize = 0;
    for (resymb = next; resymb && resymb->op == EOps::ReSymb; resymb = resymb->next, wsize++) {
    }
    if (resymb && resymb->op > EOps::ReBlockOps && resymb->op < EOps::ReSymbolOps) {
      wsize--;
      resymb = resymb->prev;
    }
    if (wsize > 1) {
      reafterword = resymb;
      resymb = reword;
      auto* word = new UnicodeString();
      for (int idx = 0; idx < wsize; idx++) {
        word->append(resymb->un.symbol);
        SRegInfo* retmp = resymb;
        resymb = resymb->next;
        retmp->next = nullptr;
        if (idx > 0)
          delete retmp;
      }
      reword->op = EOps::ReWord;
      reword->un.word = word;
      reword->next = reafterword;
      if (reafterword)
        reafterword->prev = reword;
      continue;
    }

    // adds empty alternative
    while (next->op == EOps::ReOr) {
      temp = new SRegInfo;
      temp->parent = next->parent;
      // |foo|bar
      if (!next->prev) {
        temp->next = next;
        next->prev = temp;
        continue;
      }
      // foo||bar
      if (next->next && next->next->op == EOps::ReOr) {
        temp->prev = next;
        temp->next = next->next;
        if (next->next)
          next->next->prev = temp;
        next->next = temp;
        continue;
      }
      // foo|bar|
      if (!next->next) {
        temp->prev = next;
        temp->next = nullptr;
        next->next = temp;
        continue;
      }
      // foo|bar|*
      if (next->next->op > EOps::ReBlockOps && next->next->op < EOps::ReSymbolOps) {
        temp->prev = next;
        temp->next = next->next;
        next->next->prev = temp;
        next->next = temp;
        continue;
      }
      delete temp;
      break;
    }
  }

  // op's generating...
  next = re;
  SRegInfo* realFirst;
  while (next) {
    if (next->op > EOps::ReBlockOps && next->op < EOps::ReSymbolOps) {
      if (!next->prev)
        return EError::EOP;
      realFirst = next->prev;
      realFirst->next = nullptr;
      realFirst->parent = next;
      while (next->op == EOps::ReOr && realFirst->prev && realFirst->prev->op != EOps::ReOr) {
        realFirst->parent = next;
        realFirst = realFirst->prev;
      }

      if (!realFirst->prev) {
        re = next;
        next->un.param = realFirst;
        next->prev = nullptr;
      }
      else {
        next->un.param = realFirst;
        next->prev = realFirst->prev;
        realFirst->prev->next = next;
      }
      realFirst->prev = nullptr;
    }
    next = next->next;
  }
  if (retPos == -1)
    retPos = to - from;
  return EError::EOK;
}

////////////////////////////////////////////////////////////////////////////
// parsing
////////////////////////////////////////////////////////////////////////////

static bool isLineBreak(wchar c)
{
   return c == 0x0A || c == 0x0B || c == 0x0C || c == 0x0D || c == 0x85 || c == 0x2028 || c == 0x2029;
}

bool CRegExp::isWordBoundary(int toParse)
{
  const bool after = (toParse < end && Character::isLetterOrDigitOrUnderscore((*global_pattern)[toParse]));
  const bool before = (toParse > 0 && Character::isLetterOrDigitOrUnderscore((*global_pattern)[toParse - 1]));
  return before != after;
}

bool CRegExp::checkMetaSymbol(EMetaSymbols symb, int& toParse)
{
  const UnicodeString& pattern = *global_pattern;

  switch (symb) {
    case EMetaSymbols::ReAnyChr:
      if (toParse >= end || (!singleLine && isLineBreak(pattern[toParse])))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReSoL:
        return toParse == 0 || (multiLine && isLineBreak(pattern[toParse - 1]));

    case EMetaSymbols::ReEoL:
      return toParse == end || (multiLine && toParse && toParse < end && isLineBreak(pattern[toParse - 1]));

    case EMetaSymbols::ReDigit:
      if (toParse >= end || !Character::isDigit(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNDigit:
      if (toParse >= end || Character::isDigit(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWordSymb:
      if (toParse >= end || !Character::isLetterOrDigitOrUnderscore(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNWordSymb:
      if (toParse >= end || Character::isLetterOrDigitOrUnderscore(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWSpace:
      if (toParse >= end || !Character::isWhitespace(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNWSpace:
      if (toParse >= end || Character::isWhitespace(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReUCase:
      if (toParse >= end || !Character::isUpperCase(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNUCase:
      if (toParse >= end || !Character::isLowerCase(pattern[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWBound:
      return isWordBoundary(toParse);

    case EMetaSymbols::ReNWBound:
      return !isWordBoundary(toParse);

    case EMetaSymbols::RePreNW:
      return toParse == 0 || toParse >= end || !Character::isLetter(pattern[toParse - 1]);

#ifdef COLORERMODE
    case EMetaSymbols::ReSoScheme:
      return (schemeStart == toParse);

    case EMetaSymbols::ReStart:
      matches->s[0] = toParse;
      startChange = true;
      return true;

    case EMetaSymbols::ReEnd:
      matches->e[0] = toParse;
      endChange = true;
      return true;
#endif

    default:
      return false;
  }
}

void CRegExp::check_stack(bool res, SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter, ReAction* action)
{
  if (count_elem == 0) {
    *action = res ? rea_True : rea_False;
    return;
  }

  StackElem& ne = CRegExp::RegExpStack[--count_elem];
  if (res) {
    *action = ne.ifTrueReturn;
  }
  else {
    *action = ne.ifFalseReturn;
  }
  *re = ne.re;
  *prev = ne.prev;
  *toParse = ne.toParse;
  *leftenter = ne.leftenter;
}

bool CRegExp::matchCopiedRange(const UnicodeString& src, int from, int to, int& toParse, bool icase) const
{
  // Unmatched groups are stored as -1,-1; the classic copy loop then does nothing.
  if (from < 0 || to < 0)
    return true;
  const UnicodeString& pattern = *global_pattern;
  for (int i = from; i < to; i++) {
    if (toParse >= end)
      return false;
    if (icase) {
      if (Character::toLowerCase(pattern[toParse]) != Character::toLowerCase(src[i]))
        return false;
    }
    else if (pattern[toParse] != src[i]) {
      return false;
    }
    toParse++;
  }
  return true;
}

void CRegExp::insert_stack(SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter, ReAction ifTrueReturn,
                           ReAction ifFalseReturn, SRegInfo** re2, SRegInfo** prev2, int toParse2)
{
  if (RegExpStack.size() == static_cast<size_t>(count_elem)) {
    RegExpStack.resize(RegExpStack.empty() ? INIT_MEM_SIZE : RegExpStack.size() + MEM_INC);
  }
  StackElem& ne = CRegExp::RegExpStack[static_cast<size_t>(count_elem++)];
  ne.re = *re;
  ne.prev = *prev;
  ne.toParse = *toParse;
  ne.ifTrueReturn = ifTrueReturn;
  ne.ifFalseReturn = ifFalseReturn;
  ne.leftenter = *leftenter;

  if (prev2 == nullptr)
    *prev = nullptr;
  else
    *prev = *prev2;
  *re = *re2;
  *toParse = toParse2;
  // this is init operation from lowParse
  *leftenter = true;
  if (!*re) {
    *re = (*prev)->parent;
    *leftenter = false;
  }
}

bool CRegExp::lowParse(SRegInfo* re, SRegInfo* prev, int toParse)
{
  int i, sv, wlen;
  bool leftenter = true;
  bool br = false;
  const UnicodeString& pattern = *global_pattern;
  ReAction action = rea_None;

  if (!re) {
    re = prev->parent;
    leftenter = false;
  }
  while (true) {
    while (re || action != rea_None) {
      if (++parseSteps > parseStepLimit) {
        stepBudgetExceeded = true;
        return false;
      }
      if (re && action == rea_None)
        switch (re->op) {
          case EOps::ReEmpty:
            break;
          case EOps::ReBrackets:
          case EOps::ReNamedBrackets:
            if (leftenter) {
              re->s = toParse;
              re = re->un.param;
              continue;
            }
            if (re->param0 == -1)
              break;
            if (re->op == EOps::ReBrackets) {
              matches->topseSanitize(re->param0);
              if (re->param0 || !startChange)
                matches->s[re->param0] = re->s;
              if (re->param0 || !endChange)
                matches->e[re->param0] = toParse;
              if (matches->e[re->param0] < matches->s[re->param0])
                matches->s[re->param0] = matches->e[re->param0];
            }
            else {
              matches->topnseSanitize(re->param0);
              matches->ns[re->param0] = re->s;
              matches->ne[re->param0] = toParse;
              if (matches->ne[re->param0] < matches->ns[re->param0])
                matches->ns[re->param0] = matches->ne[re->param0];
            }
            break;
          case EOps::ReSymb:
            if (toParse >= end) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (!matchChars(pattern[toParse], re->un.symbol)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            toParse++;
            break;
          case EOps::ReMetaSymb:
            if (!checkMetaSymbol(re->un.metaSymbol, toParse)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            break;
          case EOps::ReWord:
            wlen = re->un.word->length();
            if (toParse + wlen > end) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (ignoreCase) {
              if (UStr::caseCompare(pattern, toParse, wlen, *re->un.word) != 0) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                continue;
              }
            }
            else {
              if (pattern.compare(toParse, wlen, *re->un.word) != 0) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                continue;
              }
            }
            toParse += wlen;
            break;
          case EOps::ReEnum:
            if (toParse >= end) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (!re->un.charclass->contains(pattern[toParse])) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            toParse++;
            break;
#ifdef COLORERMODE
          case EOps::ReBkTrace:
            sv = re->param0;
            if (!backStr || !backTrace || sv < 0 || sv >= MATCHES_NUM) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            backTrace->topseSanitize(sv);
            if (!matchCopiedRange(*backStr, backTrace->s[sv], backTrace->e[sv], toParse, false)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            break;
          case EOps::ReBkTraceN:
            sv = re->param0;
            if (!backStr || !backTrace || sv < 0 || sv >= MATCHES_NUM) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            backTrace->topseSanitize(sv);
            if (!matchCopiedRange(*backStr, backTrace->s[sv], backTrace->e[sv], toParse, true)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            break;
          case EOps::ReBkTraceName:
            sv = re->param0;
            if (!backStr || !backTrace || sv < 0 || sv >= NAMED_MATCHES_NUM) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            backTrace->topnseSanitize(sv);
            if (!matchCopiedRange(*backStr, backTrace->ns[sv], backTrace->ne[sv], toParse, false)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            break;
          case EOps::ReBkTraceNName:
            sv = re->param0;
            if (!backStr || !backTrace || sv < 0 || sv >= NAMED_MATCHES_NUM || backTrace->cnMatch <= sv) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            backTrace->topnseSanitize(sv);
            if (backTrace->ns[sv] == -1 || backTrace->ne[sv] == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (!matchCopiedRange(*backStr, backTrace->ns[sv], backTrace->ne[sv], toParse, true)) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            break;
#endif  // COLORERMODE

          case EOps::ReBkBrackName:
            sv = re->param0;
            if (sv == -1 || cnMatch <= sv) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            matches->topnseSanitize(sv);
            if (matches->ns[sv] == -1 || matches->ne[sv] == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = matches->ns[sv]; i < matches->ne[sv]; i++) {
              if (toParse >= end || pattern[toParse] != pattern[i]) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;

          case EOps::ReBkBrack:
            sv = re->param0;
            if (sv == -1 || cMatch <= sv) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            matches->topseSanitize(sv);
            if (matches->s[sv] == -1 || matches->e[sv] == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = matches->s[sv]; i < matches->e[sv]; i++) {
              if (toParse >= end || pattern[toParse] != pattern[i]) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;
          case EOps::ReAhead:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_Break, rea_False, &re->un.param, nullptr, toParse);
              continue;
            }
            break;
          case EOps::ReNAhead:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_False, rea_Break, &re->un.param, nullptr, toParse);
              continue;
            }
            break;
          case EOps::ReBehind:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (toParse - re->param0 < 0) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            else {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_Break, rea_False, &re->un.param, nullptr,
                           toParse - re->param0);
              continue;
            }
            break;
          case EOps::ReNBehind:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (toParse - re->param0 >= 0) {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_False, rea_Break, &re->un.param, nullptr,
                           toParse - re->param0);
              continue;
            }
            break;

          case EOps::ReOr:
            if (!leftenter) {
              while (re->next) re = re->next;
              break;
            }
            {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_Break, &re->un.param, nullptr, toParse);
              continue;
            }
            break;
          case EOps::ReRangeN:
            // first enter into op
            if (leftenter) {
              re->param0 = re->s;
              re->oldParse = -1;
            }
            if (!re->param0 && re->oldParse == toParse)
              break;
            re->oldParse = toParse;
            // making branch
            if (!re->param0) {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeN_step2, &re->un.param, nullptr,
                           toParse);
              continue;
            }
            else {
              // go into
              re->param0--;
            }
            re = re->un.param;
            leftenter = true;
            continue;
          case EOps::ReRangeNM:
            if (leftenter) {
              re->param0 = re->s;
              re->param1 = re->e - re->s;
              re->oldParse = -1;
            }
            if (!re->param0) {
              if (re->param1)
                re->param1--;
              else {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next, &re, toParse);
                continue;
              }
              {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeNM_step2, &re->un.param, nullptr,
                             toParse);
                continue;
              }
            }
            else
              re->param0--;
            re = re->un.param;
            leftenter = true;
            continue;
          case EOps::ReNGRangeN:
            if (leftenter) {
              re->param0 = re->s;
              re->oldParse = -1;
            }
            if (!re->param0 && re->oldParse == toParse)
              break;
            re->oldParse = toParse;
            if (!re->param0) {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeN_step2, &re->next, &re, toParse);
              continue;
            }
            else
              re->param0--;
            re = re->un.param;
            leftenter = true;
            continue;
          case EOps::ReNGRangeNM:
            if (leftenter) {
              re->param0 = re->s;
              re->param1 = re->e - re->s;
              re->oldParse = -1;
            }
            if (!re->param0) {
              if (re->param1)
                re->param1--;
              else {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next, &re, toParse);
                continue;
              }
              {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeNM_step2, &re->next, &re, toParse);
                continue;
              }
            }
            else
              re->param0--;
            re = re->un.param;
            leftenter = true;
            continue;
          case EOps::ReBlockOps:
          case EOps::ReSymbolOps:
            break;
        }

      switch (action) {
        case rea_None:
          break;
        case rea_False:
          if (count_elem) {
            check_stack(false, &re, &prev, &toParse, &leftenter, &action);
            continue;
          }
          else
            return false;
          break;
        case rea_True:
          if (count_elem) {
            check_stack(true, &re, &prev, &toParse, &leftenter, &action);
            continue;
          }
          else
            return true;
          break;
        case rea_Break:
          action = rea_None;
          break;
        case rea_RangeN_step2:
          action = rea_None;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next, &re, toParse);
          continue;
          break;
        case rea_RangeNM_step2:
          action = rea_None;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeNM_step3, &re->next, &re, toParse);
          continue;
          break;
        case rea_RangeNM_step3:
          action = rea_None;
          re->param1++;
          check_stack(false, &re, &prev, &toParse, &leftenter, &action);
          continue;
          break;
        case rea_NGRangeN_step2:
          action = rea_None;
          if (re->param0)
            re->param0--;
          re = re->un.param;
          leftenter = true;
          continue;
          break;
        case rea_NGRangeNM_step2:
          action = rea_None;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeNM_step3, &re->un.param, nullptr,
                       toParse);
          continue;
          break;
        case rea_NGRangeNM_step3:
          action = rea_None;
          re->param1++;
          check_stack(false, &re, &prev, &toParse, &leftenter, &action);
          continue;
          break;
      }
      if (!re->next) {
        re = re->parent;
        leftenter = false;
      }
      else {
        re = re->next;
        leftenter = true;
      }
    }
    check_stack(true, &re, &prev, &toParse, &leftenter, &action);
  }
}

bool CRegExp::canStartWith(wchar ch) const
{
  const auto value = static_cast<uint32_t>(ch);
  if (firstCharMaskUseful && value < 128) {
    return (firstCharMask[value >> 6] & (uint64_t(1) << (value & 63))) != 0;
  }
  if (!firstNode) {
    return true;
  }
  switch (firstNode->op) {
    case EOps::ReSymb:
      return matchChars(ch, firstNode->un.symbol);
    case EOps::ReWord:
      return matchChars(ch, (*firstNode->un.word)[0]);
    case EOps::ReEnum:
      return firstNode->un.charclass->contains(ch);
    case EOps::ReMetaSymb:
      switch (firstNode->un.metaSymbol) {
        case EMetaSymbols::ReAnyChr:
          return singleLine || !isLineBreak(ch);
        case EMetaSymbols::ReDigit:
          return Character::isDigit(ch);
        case EMetaSymbols::ReNDigit:
          return !Character::isDigit(ch);
        case EMetaSymbols::ReWordSymb:
          return Character::isLetterOrDigitOrUnderscore(ch);
        case EMetaSymbols::ReNWordSymb:
          return !Character::isLetterOrDigitOrUnderscore(ch);
        case EMetaSymbols::ReWSpace:
          return Character::isWhitespace(ch);
        case EMetaSymbols::ReNWSpace:
          return !Character::isWhitespace(ch);
        case EMetaSymbols::ReUCase:
          return Character::isUpperCase(ch);
        case EMetaSymbols::ReNUCase:
          return Character::isLowerCase(ch);
        default:
          return true;
      }
    default:
      return true;
  }
}

inline bool CRegExp::quickCheck(int toParse)
{
  switch (firstNode->op) {
    case EOps::ReSymb:
      return toParse < end && matchChars((*global_pattern)[toParse], firstNode->un.symbol);
    case EOps::ReWord:
      return toParse < end && matchChars((*global_pattern)[toParse], (*firstNode->un.word)[0]);
    case EOps::ReEnum:
      return toParse < end && firstNode->un.charclass->contains((*global_pattern)[toParse]);
    case EOps::ReMetaSymb:
      switch (firstNode->un.metaSymbol) {
#ifdef COLORERMODE
        case EMetaSymbols::ReStart:
        case EMetaSymbols::ReEnd:
          return true;
#endif
        case EMetaSymbols::ReBadMeta:
        case EMetaSymbols::ReChrLast:
          return true;
        default:
          return checkMetaSymbol(firstNode->un.metaSymbol, toParse);
      }
    default:
      return true;
  }
}

inline bool CRegExp::parseRE(int pos)
{
  if (error != EError::EOK)
    return false;

  count_elem = 0;
  parseSteps = 0;
  stepBudgetExceeded = false;
  // Early-out paths never reach the search loop; still drop \m/\M left on a
  // reused end-RE from the previous parse().
  startChange = false;
  endChange = false;
  int toParse = pos;

  if (!positionMoves && firstCharMaskUseful) {
    if (toParse >= end) return false;
    const auto ch = static_cast<uint32_t>((*global_pattern)[toParse]);
    if (ch < 128 && !(firstCharMask[ch >> 6] & (uint64_t(1) << (ch & 63)))) return false;
  }
  if (!positionMoves && firstNode && !quickCheck(toParse))
    return false;

  matches->reset();
  matches->cMatch = cMatch;
  matches->cnMatch = cnMatch;
  do {
    if (positionMoves) {
      bool skip = false;
      if (firstCharMaskUseful) {
        if (toParse >= end) {
          skip = true;
        }
        else {
          const auto ch = static_cast<uint32_t>((*global_pattern)[toParse]);
          if (ch < 128 && !(firstCharMask[ch >> 6] & (uint64_t(1) << (ch & 63))))
            skip = true;
        }
      }
      if (!skip && firstNode && !quickCheck(toParse))
        skip = true;
      if (skip) {
        toParse = ++pos;
        continue;
      }
    }
    startChange = false;
    endChange = false;
    matches->reset();
    matches->cMatch = cMatch;
    matches->cnMatch = cnMatch;
    if (lowParse(tree_root, nullptr, toParse)) {
      matches->topseSanitize(cMatch - 1);
      matches->topnseSanitize(cnMatch - 1);
      return true;
    }
    if (stepBudgetExceeded)
      return false;
    if (!positionMoves)
      return false;
    toParse = ++pos;
  } while (toParse <= end);
  return false;
}

bool CRegExp::parse(const UnicodeString* str, int pos, int eol, SMatches* mtch, int soScheme,
                    int posMoves)
{
  bool nms = positionMoves;
  if (posMoves != -1)
    positionMoves = (posMoves != 0);
#ifdef COLORERMODE
  schemeStart = soScheme;
#endif
  global_pattern = str;
  end = eol;
  matches = mtch;
  bool result = parseRE(pos);
  positionMoves = nms;
  return result;
}

bool CRegExp::parse(const UnicodeString* str, SMatches* mtch)
{
  end = str->length();
  global_pattern = str;
#ifdef COLORERMODE
  schemeStart = 0;
#endif
  matches = mtch;
  return parseRE(0);
}

/////////////////////////////////////////////////////////////////
// other methods

bool CRegExp::setRE(const UnicodeString* re)
{
  error = EError::EERROR;
  error = setRELow(*re);
  return error == EError::EOK;
}
bool CRegExp::isOk() const
{
  return error == EError::EOK;
}
EError CRegExp::getError() const
{
  return error;
}

bool CRegExp::setPositionMoves(bool moves)
{
  positionMoves = moves;
  return true;
}

void CRegExp::setParseStepLimit(int limit)
{
  parseStepLimit = limit < 1 ? 1 : limit;
}

int CRegExp::getParseStepLimit() const
{
  return parseStepLimit;
}

bool CRegExp::exceededParseStepLimit() const
{
  return stepBudgetExceeded;
}

void CRegExp::clearRegExpStack()
{
  std::vector<StackElem>().swap(RegExpStack);
}

int CRegExp::getBracketNo(const UnicodeString* brname) const
{
  for (int brn = 0; brn < cnMatch; brn++)
    if (UStr::caseCompare(*brname, *brnames[brn]) == 0)
      return brn;
  return -1;
}
const UnicodeString* CRegExp::getBracketName(int no) const
{
  if (no >= cnMatch)
    return nullptr;
  return brnames[no];
}

#ifdef COLORERMODE
bool CRegExp::setBackRE(CRegExp* bkre)
{
  this->backRE = bkre;
  return true;
}
bool CRegExp::setBackTrace(const UnicodeString* str, SMatches* trace)
{
  backTrace = trace;
  backStr = str;
  return true;
}
bool CRegExp::getBackTrace(const UnicodeString** str, SMatches** trace) const
{
  *str = backStr;
  *trace = backTrace;
  return true;
}

bool CRegExp::hasBackTrace() const
{
  return usesBackTrace;
}

#endif
