#include "colorer/cregexp/cregexp.h"
#include <cstring>

StackElem* CRegExp::RegExpStack {nullptr};
int CRegExp::RegExpStack_Size {0};
/////////////////////////////////////////////////////////////////////////////
//
SRegInfo::SRegInfo()
{
  un.param = nullptr;
}
SRegInfo::~SRegInfo()
{
  delete next;
  if (un.param)
    switch (op) {
      case EOps::ReEnum:
      case EOps::ReNEnum:
        delete un.charclass;
        break;
      case EOps::ReWord:
        delete un.word;
        break;
#ifdef NAMED_MATCHES_IN_HASH
      case EOps::ReNamedBrackets:
      case EOps::ReBkBrackName:
        if (namedata)
          delete namedata;
#endif
      default:
        if (op > EOps::ReBlockOps &&
            (op < EOps::ReSymbolOps || op == EOps::ReBrackets || op == EOps::ReNamedBrackets))
          delete un.param;
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
  firstChar = 0;
  cMatch = 0;
  global_pattern = nullptr;
#ifdef COLORERMODE
  backRE = nullptr;
  backStr = nullptr;
  backTrace = nullptr;
#endif
#ifndef NAMED_MATCHES_IN_HASH
  cnMatch = 0;
#else
  namedMatches = 0;
#endif
  count_elem = 0;
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
#ifndef NAMED_MATCHES_IN_HASH
  for (int bp = 0; bp < cnMatch; bp++) delete brnames[bp];
#endif
}

EError CRegExp::setRELow(const UnicodeString& expr)
{
  auto len = expr.length();
  if (!len)
    return EError::EERROR;

  delete tree_root;
  tree_root = nullptr;
#ifndef NAMED_MATCHES_IN_HASH
  for (int bp = 0; bp < cnMatch; bp++) delete brnames[bp];
#endif

  cMatch = 0;
#ifndef NAMED_MATCHES_IN_HASH
  cnMatch = 0;
#endif
  endChange = startChange = false;
  int start = 0;
  while (Character::isWhitespace(expr[start])) start++;
  if (expr[start] == '/')
    start++;
  else
    return EError::ESYNTAX;

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
  EError err = setStructs(tree_root->un.param, UnicodeString(expr, start, len), endPos);
  if (endPos != len)
    err = EError::EBRACKETS;

  if (err != EError::EOK)
    return err;
  optimize();
  return EError::EOK;
}

void CRegExp::optimize()
{
  SRegInfo* next = tree_root;
  firstChar = BAD_WCHAR;
  firstMetaChar = EMetaSymbols::ReBadMeta;
  while (next) {
    if (next->op == EOps::ReBrackets) {
      next = next->un.param;
      continue;
    }
    /*    if (next->op == EOps::ReMetaSymb &&
            next->un.metaSymbol >= ReWBound && next->un.metaSymbol < ReChrLast){
          next = next->next;
          continue;
        };*/
    if (next->op == EOps::ReMetaSymb) {
      if (next->un.metaSymbol != EMetaSymbols::ReSoL &&
          next->un.metaSymbol != EMetaSymbols::ReWBound)
        break;
      firstMetaChar = next->un.metaSymbol;
      break;
    }
    if (next->op == EOps::ReSymb) {
      firstChar = next->un.symbol;
      break;
    }
    if (next->op == EOps::ReWord) {
      firstChar = (*next->un.word)[0];
    }
    break;
  }
}

EError CRegExp::setStructs(SRegInfo*& re, const UnicodeString& expr, int& retPos)
{
  SRegInfo *next, *temp;

  retPos = 0;
  if (!expr.length())
    return EError::EOK;
  retPos = -1;

  next = re;
  for (int i = 0; i < expr.length(); i++) {
    // simple character
    if (extend && Character::isWhitespace(expr[i]))
      continue;
    // context return
    if (expr[i] == ')') {
      retPos = i;
      break;
    }
    // next element
    if (i != 0) {
      next->next = new SRegInfo;
      next->next->parent = next->parent;
      next->next->prev = next;
      next = next->next;
    }
    // Escape symbol
    if (expr[i] == '\\') {
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
#ifndef NAMED_MATCHES_IN_HASH
        case 'y':
        case 'Y':
          next->op = (expr[i + 1] == 'y' ? EOps::ReBkTrace : EOps::ReBkTraceN);
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
#endif  // NAMED_MATCHES_IN_HASH

        case 'p':  // \p{name}
        {
          next->op = EOps::ReBkBrackName;
          auto br_name = UnicodeTools::getCurlyContent(expr, i + 2);
          if (br_name == nullptr)
            return EError::ESYNTAX;
          blen = br_name->length();
#ifndef NAMED_MATCHES_IN_HASH
          next->param0 = getBracketNo(br_name.get());
          if (next->param0 == -1)
            return EError::ESYNTAX;
#else
          if (br_name->length() && namedMatches && !namedMatches->getItem(br_name)) {
            return EBRACKETS;
          }
          next->param0 = 0;
          next->namedata = new UnicodeString(br_name);
#endif
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

    if (expr.length() > i + 2) {
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
    if (expr.length() > i + 1) {
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
      for (j = i; j < expr.length(); j++) {
        if (expr.length() > j + 1 && expr[j] == '}' && expr[j + 1] == '?') {
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
      UnicodeString ds = UnicodeString(expr, st, comma - st);
      next->s = UnicodeTools::getNumber(&ds);
      UnicodeString de = UnicodeString(expr, comma + 1, en - comma - 1);
      if (comma != en)
        next->e = UnicodeTools::getNumber(&de);
      else
        next->e = next->s;
      if (next->e == -1)
        return EError::EOP;
      next->un.param = nullptr;
      if (en - comma == 1)
        next->e = -1;
      if (next->e == -1)
        next->op = nonGreedy ? EOps::ReNGRangeN : EOps::ReRangeN;
      else
        next->op = nonGreedy ? EOps::ReNGRangeNM : EOps::ReRangeNM;
      i = j;
      continue;
    }
    // ( ... )
    if (expr[i] == '(') {
      // bool namedBracket = false;
      // perl-like "uncaptured" brackets
      if (expr.length() >= i + 2 && expr[i + 1] == '?' && expr[i + 2] == ':') {
        next->op = EOps::ReNamedBrackets;
        next->param0 = -1;
        // namedBracket = true;
        i += 3;
      }
      else if (expr.length() > i + 2 && expr[i + 1] == '?' && expr[i + 2] == '{') {
        // named bracket
        next->op = EOps::ReNamedBrackets;
        // namedBracket = true;
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
#ifndef NAMED_MATCHES_IN_HASH
#ifdef CHECKNAMES
          if (getBracketNo(br_name) != -1) {
            delete br_name;
            return EError::EBRACKETS;
          }
#endif
          if (cnMatch < NAMED_MATCHES_NUM) {
            next->param0 = cnMatch;
            brnames[cnMatch] = br_name;
            cnMatch++;
          }
          else
            delete br_name;
#else
#ifdef CHECKNAMES
          if (br_name->length() && namedMatches && namedMatches->getItem(br_name)) {
            delete br_name;
            return EError::EBRACKETS;
          }
#endif
          next->param0 = 0;
          next->namedata = br_name;
          if (namedMatches) {
            SMatch mt = {-1, -1};
            namedMatches->setItem(br_name, mt);
          }
#endif
        }
        i += blen + 4;
      }
      else {
        next->op = EOps::ReBrackets;
        if (cMatch < MATCHES_NUM) {
          next->param0 = cMatch;
          cMatch++;
        }
        i += 1;
      }
      next->un.param = new SRegInfo;
      next->un.param->parent = next;
      int endPos;
      EError err = setStructs(next->un.param, UnicodeString(expr, i), endPos);
      if (expr.length() - i - endPos == 0)
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
      //      next->op = (exprn[i] == ReEnumS) ? ReEnum : ReNEnum;
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
      UChar* wcword = new UChar[wsize];
      for (int idx = 0; idx < wsize; idx++) {
        wcword[idx] = resymb->un.symbol;
        SRegInfo* retmp = resymb;
        resymb = resymb->next;
        retmp->next = nullptr;
        if (idx > 0)
          delete retmp;
      }
      reword->op = EOps::ReWord;
      reword->un.word = new UnicodeString(wcword, wsize);
      delete[] wcword;
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
    retPos = expr.length();
  return EError::EOK;
}

////////////////////////////////////////////////////////////////////////////
// parsing
////////////////////////////////////////////////////////////////////////////

bool CRegExp::isWordBoundary(int toParse)
{
  int before = 0;
  int after = 0;
  if (toParse < end &&
      (Character::isLetterOrDigit((*global_pattern)[toParse]) || (*global_pattern)[toParse] == '_'))
    after = 1;
  if (toParse > 0 &&
      (Character::isLetterOrDigit((*global_pattern)[toParse - 1]) ||
       (*global_pattern)[toParse - 1] == '_'))
    before = 1;
  return before + after == 1;
}
bool CRegExp::isNWordBoundary(int toParse)
{
  return !isWordBoundary(toParse);
}

bool CRegExp::checkMetaSymbol(EMetaSymbols symb, int& toParse)
{
  const UnicodeString& pattern = *global_pattern;

  switch (symb) {
    case EMetaSymbols::ReAnyChr:
      if (toParse >= end)
        return false;
      if (!singleLine &&
          (pattern[toParse] == 0x0A || pattern[toParse] == 0x0B || pattern[toParse] == 0x0C ||
           pattern[toParse] == 0x0D || pattern[toParse] == 0x85 || pattern[toParse] == 0x2028 ||
           pattern[toParse] == 0x2029))
        return false;
      toParse++;
      return true;
    case EMetaSymbols::ReSoL:
      if (multiLine) {
        bool ok = false;
        if (toParse &&
            (pattern[toParse - 1] == 0x0A || pattern[toParse - 1] == 0x0B ||
             pattern[toParse - 1] == 0x0C || pattern[toParse - 1] == 0x0D ||
             pattern[toParse - 1] == 0x85 || pattern[toParse - 1] == 0x2028 ||
             pattern[toParse - 1] == 0x2029))
          ok = true;
        return (toParse == 0 || ok);
      }
      return (toParse == 0);
    case EMetaSymbols::ReEoL:
      if (multiLine) {
        bool ok = false;  // ???check
        if (toParse && toParse < end &&
            (pattern[toParse - 1] == 0x0A || pattern[toParse - 1] == 0x0B ||
             pattern[toParse - 1] == 0x0C || pattern[toParse - 1] == 0x0D ||
             pattern[toParse - 1] == 0x85 || pattern[toParse - 1] == 0x2028 ||
             pattern[toParse - 1] == 0x2029))
          ok = true;
        return (toParse == end || ok);
      }
      return (end == toParse);
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
      if (toParse >= end ||
          !(Character::isLetterOrDigit(pattern[toParse]) || pattern[toParse] == '_'))
        return false;
      toParse++;
      return true;
    case EMetaSymbols::ReNWordSymb:
      if (toParse >= end || Character::isLetterOrDigit(pattern[toParse]) || pattern[toParse] == '_')
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
      return isNWordBoundary(toParse);
    case EMetaSymbols::RePreNW:
      if (toParse >= end)
        return true;
      return toParse == 0 || !Character::isLetter(pattern[toParse - 1]);
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

void CRegExp::check_stack(bool res, SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter,
                          int* action)
{
  if (count_elem == 0) {
    *action = res;
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

void CRegExp::insert_stack(SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter,
                           int ifTrueReturn, int ifFalseReturn, SRegInfo** re2, SRegInfo** prev2,
                           int toParse2)
{
  if (RegExpStack_Size == 0) {
    CRegExp::RegExpStack = new StackElem[INIT_MEM_SIZE];
    RegExpStack_Size = INIT_MEM_SIZE;
  }
  if (RegExpStack_Size == count_elem) {
    RegExpStack_Size += MEM_INC;
    StackElem* s = new StackElem[RegExpStack_Size];
    memcpy(s, CRegExp::RegExpStack, count_elem * sizeof(StackElem));
    delete[] CRegExp::RegExpStack;
    CRegExp::RegExpStack = s;
  }
  StackElem& ne = CRegExp::RegExpStack[count_elem++];
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
  int action = -1;

  if (!re) {
    re = prev->parent;
    leftenter = false;
  }
  while (true) {
    while (re || action != -1) {
      if (re && action == -1)
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
              if (re->param0 || !startChange)
                matches->s[re->param0] = re->s;
              if (re->param0 || !endChange)
                matches->e[re->param0] = toParse;
              if (matches->e[re->param0] < matches->s[re->param0])
                matches->s[re->param0] = matches->e[re->param0];
            }
            else {
#ifndef NAMED_MATCHES_IN_HASH
              matches->ns[re->param0] = re->s;
              matches->ne[re->param0] = toParse;
              if (matches->ne[re->param0] < matches->ns[re->param0])
                matches->ns[re->param0] = matches->ne[re->param0];
#else
              SMatch mt = {re->s, toParse};
              namedMatches->setItem(re->namedata, mt);
#endif
            }
            break;
          case EOps::ReSymb:
            if (toParse >= end) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (ignoreCase) {
              if (Character::toLowerCase(pattern[toParse]) !=
                      Character::toLowerCase(re->un.symbol) &&
                  Character::toUpperCase(pattern[toParse]) != Character::toUpperCase(re->un.symbol))
              {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                continue;
              }
            }
            else if (pattern[toParse] != re->un.symbol) {
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
              if (UStr::caseCompare(UnicodeString(pattern, toParse, wlen),*re->un.word)!=0) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                continue;
              }
              toParse += wlen;
            }
            else {
              br = false;
              for (i = 0; i < wlen; i++) {
                if (pattern[toParse + i] != (*re->un.word)[i]) {
                  check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                  br = true;
                  break;
                }
              }
              if (br)
                continue;
              toParse += wlen;
            }
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
          case EOps::ReNEnum:
            if (toParse >= end) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (re->un.charclass->contains(pattern[toParse])) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            toParse++;
            break;
#ifdef COLORERMODE
          case EOps::ReBkTrace:
            sv = re->param0;
            if (!backStr || !backTrace || sv == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = backTrace->s[sv]; i < backTrace->e[sv]; i++) {
              if (toParse >= end || pattern[toParse] != (*backStr)[i]) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;
          case EOps::ReBkTraceN:
            sv = re->param0;
            if (!backStr || !backTrace || sv == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = backTrace->s[sv]; i < backTrace->e[sv]; i++) {
              if (toParse >= end ||
                  Character::toLowerCase(pattern[toParse]) != Character::toLowerCase((*backStr)[i]))
              {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;
          case EOps::ReBkTraceName:
#ifndef NAMED_MATCHES_IN_HASH
            sv = re->param0;
            if (!backStr || !backTrace || sv == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = backTrace->ns[sv]; i < backTrace->ne[sv]; i++) {
              if (toParse >= end || pattern[toParse] != (*backStr)[i]) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;
#else
            // !!!;
            {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
#endif  // NAMED_MATCHES_IN_HASH
          case EOps::ReBkTraceNName:
#ifndef NAMED_MATCHES_IN_HASH
            sv = re->param0;
            if (!backStr || !backTrace || sv == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = backTrace->s[sv]; i < backTrace->e[sv]; i++) {
              if (toParse >= end ||
                  Character::toLowerCase(pattern[toParse]) != Character::toLowerCase((*backStr)[i]))
              {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
            break;
#else
            // !!;
            {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
#endif  // NAMED_MATCHES_IN_HASH
#endif  // COLORERMODE

          case EOps::ReBkBrackName:
#ifndef NAMED_MATCHES_IN_HASH
            sv = re->param0;
            if (sv == -1 || cnMatch <= sv) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
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
#else
          {
            SMatch* mt = namedMatches->getItem(re->namedata);
            if (!mt) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (mt->s == -1 || mt->e == -1) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            br = false;
            for (i = mt->s; i < mt->e; i++) {
              if (toParse >= end || pattern[toParse] != pattern[i]) {
                check_stack(false, &re, &prev, &toParse, &leftenter, &action);
                br = true;
                break;
              }
              toParse++;
            }
            if (br)
              continue;
          } break;
#endif  // NAMED_MATCHES_IN_HASH

          case EOps::ReBkBrack:
            sv = re->param0;
            if (sv == -1 || cMatch <= sv) {
              check_stack(false, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
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
              insert_stack(&re, &prev, &toParse, &leftenter, rea_Break, rea_False, &re->un.param,
                           nullptr, toParse);
              continue;
            }
            break;
          case EOps::ReNAhead:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_False, rea_Break, &re->un.param,
                           nullptr, toParse);
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
              insert_stack(&re, &prev, &toParse, &leftenter, rea_Break, rea_False, &re->un.param,
                           nullptr, toParse - re->param0);
              continue;
            }
            break;
          case EOps::ReNBehind:
            if (!leftenter) {
              check_stack(true, &re, &prev, &toParse, &leftenter, &action);
              continue;
            }
            if (toParse - re->param0 >= 0) {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_False, rea_Break, &re->un.param,
                           nullptr, toParse - re->param0);
              continue;
            }
            break;

          case EOps::ReOr:
            if (!leftenter) {
              while (re->next) re = re->next;
              break;
            }
            {
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_Break, &re->un.param,
                           nullptr, toParse);
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
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeN_step2,
                           &re->un.param, nullptr, toParse);
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
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next, &re,
                             toParse);
                continue;
              }
              {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeNM_step2,
                             &re->un.param, nullptr, toParse);
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
              insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeN_step2,
                           &re->next, &re, toParse);
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
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next, &re,
                             toParse);
                continue;
              }
              {
                insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeNM_step2,
                             &re->next, &re, toParse);
                continue;
              }
            }
            else
              re->param0--;
            re = re->un.param;
            leftenter = true;
            continue;
          case EOps::ReBlockOps:
          case EOps::ReMul:
          case EOps::RePlus:
          case EOps::ReQuest:
          case EOps::ReNGMul:
          case EOps::ReNGPlus:
          case EOps::ReNGQuest:
          case EOps::ReSymbolOps:
            break;
        }

      switch (action) {
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
          action = -1;
          break;
        case rea_RangeN_step2:
          action = -1;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_False, &re->next,
                       &re,  //-V522
                       toParse);
          continue;
          break;
        case rea_RangeNM_step2:
          action = -1;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_RangeNM_step3, &re->next,
                       &re, toParse);
          continue;
          break;
        case rea_RangeNM_step3:
          action = -1;  //-V1037
          re->param1++;
          check_stack(false, &re, &prev, &toParse, &leftenter, &action);
          continue;
          break;
        case rea_NGRangeN_step2:
          action = -1;
          if (re->param0)
            re->param0--;
          re = re->un.param;
          leftenter = true;
          continue;
          break;
        case rea_NGRangeNM_step2:
          action = -1;
          insert_stack(&re, &prev, &toParse, &leftenter, rea_True, rea_NGRangeNM_step3,
                       &re->un.param, nullptr, toParse);
          continue;
          break;
        case rea_NGRangeNM_step3:
          action = -1;
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

inline bool CRegExp::quickCheck(int toParse)
{
  if (firstChar != BAD_WCHAR) {
    if (toParse >= end)
      return false;
    if (ignoreCase) {
      if (Character::toLowerCase((*global_pattern)[toParse]) != Character::toLowerCase(firstChar))
        return false;
    }
    else if ((*global_pattern)[toParse] != firstChar)
      return false;
    return true;
  }
  if (firstMetaChar != EMetaSymbols::ReBadMeta)
    switch (firstMetaChar) {
      case EMetaSymbols::ReSoL:
        if (toParse != 0)
          return false;
        return true;
#ifdef COLORERMODE
      case EMetaSymbols::ReSoScheme:
        if (toParse != schemeStart)
          return false;
        return true;
#endif
        //    case ReWBound:
        //      return relocale->cl_isword(*toParse) && (toParse == start ||
        //      !relocale->cl_isword(*(toParse-1)));
      case EMetaSymbols::ReBadMeta:
      case EMetaSymbols::ReAnyChr:
      case EMetaSymbols::ReEoL:
      case EMetaSymbols::ReDigit:
      case EMetaSymbols::ReNDigit:
      case EMetaSymbols::ReWordSymb:
      case EMetaSymbols::ReNWordSymb:
      case EMetaSymbols::ReWSpace:
      case EMetaSymbols::ReNWSpace:
      case EMetaSymbols::ReUCase:
      case EMetaSymbols::ReNUCase:
      case EMetaSymbols::ReWBound:
      case EMetaSymbols::ReNWBound:
      case EMetaSymbols::RePreNW:
      case EMetaSymbols::ReStart:
      case EMetaSymbols::ReEnd:
      case EMetaSymbols::ReChrLast:
        break;
    }
  return true;
}

inline bool CRegExp::parseRE(int pos)
{
  if (error != EError::EOK)
    return false;

  int toParse = pos;

  if (!positionMoves && (firstChar != BAD_WCHAR || firstMetaChar != EMetaSymbols::ReBadMeta) &&
      !quickCheck(toParse))
    return false;

  int i;
  for (i = 0; i < cMatch; i++) matches->s[i] = matches->e[i] = -1;
  matches->cMatch = cMatch;
#ifndef NAMED_MATCHES_IN_HASH
  for (i = 0; i < cnMatch; i++) matches->ns[i] = matches->ne[i] = -1;
  matches->cnMatch = cnMatch;
#endif
  do {
    // stack=null;
    if (lowParse(tree_root, nullptr, toParse))
      return true;
    if (!positionMoves)
      return false;
    toParse = ++pos;
  } while (toParse <= end);
  return false;
}

bool CRegExp::parse(const UnicodeString* str, int pos, int eol, SMatches* mtch
#ifdef NAMED_MATCHES_IN_HASH
                    ,
                    PMatchHash nmtch
#endif
                    ,
                    int soScheme, int posMoves)
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
#ifdef NAMED_MATCHES_IN_HASH
  namedMatches = nmtch;
#endif
  bool result = parseRE(pos);
  positionMoves = nms;
  return result;
}

bool CRegExp::parse(const UnicodeString* str, SMatches* mtch
#ifdef NAMED_MATCHES_IN_HASH
                    ,
                    PMatchHash nmtch
#endif
)
{
  end = str->length();
  global_pattern = str;
#ifdef COLORERMODE
  schemeStart = 0;
#endif
  matches = mtch;
#ifdef NAMED_MATCHES_IN_HASH
  namedMatches = nmtch;
#endif
  return parseRE(0);
}

/////////////////////////////////////////////////////////////////
// other methods

bool CRegExp::setRE(const UnicodeString* re)
{
  error = EError::EERROR;
#ifdef NAMED_MATCHES_IN_HASH
  PMatchHash oldnamedMatches = namedMatches;
  SMatchHash tmpMatchHash;
  namedMatches = &tmpMatchHash;
  error = setRELow(*re);
  namedMatches = oldnamedMatches;
#else
  error = setRELow(*re);
#endif
  return error == EError::EOK;
}
bool CRegExp::isOk()
{
  return error == EError::EOK;
}
EError CRegExp::getError()
{
  return error;
}

bool CRegExp::setPositionMoves(bool moves)
{
  positionMoves = moves;
  return true;
}

void CRegExp::clearRegExpStack()
{
  CRegExp::RegExpStack_Size = 0;
  delete[] CRegExp::RegExpStack;
  CRegExp::RegExpStack = nullptr;
}

#ifndef NAMED_MATCHES_IN_HASH
int CRegExp::getBracketNo(const UnicodeString* brname)
{
  for (int brn = 0; brn < cnMatch; brn++)
    if (UStr::caseCompare(*brname,*brnames[brn])==0)
      return brn;
  return -1;
}
UnicodeString* CRegExp::getBracketName(int no)
{
  if (no >= cnMatch)
    return nullptr;
  return brnames[no];
}
#endif

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
bool CRegExp::getBackTrace(const UnicodeString** str, SMatches** trace)
{
  *str = backStr;
  *trace = backTrace;
  return true;
}

#endif
