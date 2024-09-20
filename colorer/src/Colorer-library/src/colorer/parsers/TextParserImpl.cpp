#include "colorer/parsers/TextParserImpl.h"

TextParser::Impl::Impl()
{
  COLORER_LOG_DEEPTRACE("[TextParserImpl] constructor");
  initCache();
}

TextParser::Impl::~Impl()
{
  delete cache;
}

void TextParser::Impl::setFileType(FileType* type)
{
  baseScheme = nullptr;
  if (type != nullptr) {
    baseScheme = (SchemeImpl*) (type->getBaseScheme());
  }
  initCache();
}

void TextParser::Impl::setLineSource(LineSource* lh)
{
  lineSource = lh;
}

void TextParser::Impl::setRegionHandler(RegionHandler* rh)
{
  regionHandler = rh;
}

int TextParser::Impl::parse(int from, int num, TextParseMode mode)
{
  gx = 0;
  current_parse_line = from;
  end_line4parse = from + num;
  clearLine = -1;

  invisibleSchemesFilled = false;
  schemeStart = -1;
  breakParsing = false;
  updateCache = (mode == TextParseMode::TPM_CACHE_UPDATE);

  COLORER_LOG_DEEPTRACE("[TextParserImpl] parse from=%, num=%", from, num);
  /* Check for initial bad conditions */
  if (!regionHandler || !lineSource || !baseScheme) {
    return from;
  }

  vtlist = new VTList();

  lineSource->startJob(from);
  regionHandler->startParsing(from);

  /* Init cache */
  parent = cache;
  forward = nullptr;
  cache->scheme = baseScheme;

  if (mode == TextParseMode::TPM_CACHE_READ || mode == TextParseMode::TPM_CACHE_UPDATE) {
    parent = cache->searchLine(from, &forward);
    if (parent != nullptr) {
      COLORER_LOG_DEEPTRACE("[TPCache] searchLine() parent:%,%-%", *parent->scheme->getName(),
                           parent->sline, parent->eline);
    }
  }
  COLORER_LOG_DEEPTRACE("[TextParserImpl] parse: cache filled");

  do {
    if (!forward) {
      if (!parent) {
        return from;
      }
      if (updateCache) {
        delete parent->children;
        parent->children = nullptr;
      }
    }
    else {
      if (updateCache) {
        delete forward->next;
        forward->next = nullptr;
      }
    }
    baseScheme = parent->scheme;

    stackLevel = 0;
    COLORER_LOG_DEEPTRACE("[TextParserImpl] parse: goes into colorize()");
    if (parent != cache) {
      vtlist->restore(parent->vcache);
      parent->clender->end->setBackTrace(parent->backLine, &parent->matchstart);
      colorize(parent->clender->end.get(), parent->clender->lowContentPriority);
      vtlist->clear();
    }
    else {
      colorize(nullptr, false);
    }

    if (updateCache) {
      if (parent != cache) {
        parent->eline = current_parse_line;
      }
    }
    if (parent != cache && current_parse_line < end_line4parse) {
      leaveScheme(current_parse_line, &matchend, parent->clender);
    }
    gx = matchend.e[0];

    forward = parent;
    parent = parent->parent;
  } while (parent);
  regionHandler->endParsing(endLine);
  lineSource->endJob(endLine);
  delete vtlist;
  return endLine;
}

void TextParser::Impl::initCache()
{
  delete cache;
  cache = new ParseCache();
  cache->eline = 0x7FFFFFF;
}

void TextParser::Impl::breakParse()
{
  breakParsing = true;
}

void TextParser::Impl::addRegion(int lno, int sx, int ex, const Region* region)
{
  if (sx == -1 || region == nullptr) {
    return;
  }
  regionHandler->addRegion(lno, str, sx, ex, region);
}

void TextParser::Impl::enterScheme(int lno, int sx, int ex, const Region* region)
{
  regionHandler->enterScheme(lno, str, sx, ex, region, baseScheme);
}

void TextParser::Impl::leaveScheme(int lno, int sx, int ex, const Region* region)
{
  regionHandler->leaveScheme(lno, str, sx, ex, region, baseScheme);
}

void TextParser::Impl::enterScheme(int lno, const SMatches* match,
                                   const SchemeNodeBlock* schemeNode)
{
  if (!schemeNode->innerRegion) {
    enterScheme(lno, match->s[0], match->e[0], schemeNode->region);
  }

  for (int i = 0; i < match->cMatch; i++) {
    addRegion(lno, match->s[i], match->e[i], schemeNode->regions[i]);
  }
  for (int i = 0; i < match->cnMatch; i++) {
    addRegion(lno, match->ns[i], match->ne[i], schemeNode->regionsn[i]);
  }

  if (schemeNode->innerRegion) {
    enterScheme(lno, match->e[0], match->e[0], schemeNode->region);
  }
}

void TextParser::Impl::leaveScheme(int /*lno*/, const SMatches* match,
                                   const SchemeNodeBlock* schemeNode)
{
  if (schemeNode->innerRegion) {
    leaveScheme(current_parse_line, match->s[0], match->s[0], schemeNode->region);
  }

  for (int i = 0; i < match->cMatch; i++) {
    addRegion(current_parse_line, match->s[i], match->e[i], schemeNode->regione[i]);
  }
  for (int i = 0; i < match->cnMatch; i++) {
    addRegion(current_parse_line, match->ns[i], match->ne[i], schemeNode->regionen[i]);
  }

  if (!schemeNode->innerRegion) {
    leaveScheme(current_parse_line, match->s[0], match->e[0], schemeNode->region);
  }
}

void TextParser::Impl::fillInvisibleSchemes(ParseCache* ch)
{
  if (!ch->parent || ch == cache) {
    return;
  }
  /* Fills output stream with valid "pseudo" enterScheme */
  fillInvisibleSchemes(ch->parent);
  enterScheme(current_parse_line, 0, 0, ch->clender->region);
}

int TextParser::Impl::searchKW(const SchemeNodeKeywords* node, int /*no*/, int lowlen,
                               int /*hilen*/)
{
  if (node->kwList->count == 0 || node->kwList->minKeywordLength + gx > lowlen) {
    return MATCH_NOTHING;
  }

  if (gx < lowlen && !node->kwList->firstChar->contains((*str)[gx])) {
    return MATCH_NOTHING;
  }

  int left = 0;
  int right = node->kwList->count;
  while (true) {
    int pos = left + (right - left) / 2;
    int kwlen = node->kwList->kwList[pos].keyword->length();
    if (lowlen < gx + kwlen) {
      kwlen = lowlen - gx;
    }

    int8_t compare_result;
    if (node->kwList->matchCase) {
      compare_result = node->kwList->kwList[pos].keyword->compare(UnicodeString(*str, gx, kwlen));
    }
    else {
      compare_result =
          UStr::caseCompare(*node->kwList->kwList[pos].keyword, UnicodeString(*str, gx, kwlen));
    }

    if (compare_result == 0 && right - left == 1) {
      bool badbound = false;
      if (!node->kwList->kwList[pos].isSymbol) {
        if (!node->worddiv) {
          // default word bound
          if ((gx > 0 && (Character::isLetterOrDigit((*str)[gx - 1]) || (*str)[gx - 1] == L'_')) ||
              (gx + kwlen < lowlen &&
               (Character::isLetterOrDigit((*str)[gx + kwlen]) || (*str)[gx + kwlen] == L'_')))
          {
            badbound = true;
          }
        }
        else {
          // custom check for word bound
          if ((gx > 0 && !node->worddiv->contains((*str)[gx - 1])) ||
              (gx + kwlen < lowlen && !node->worddiv->contains((*str)[gx + kwlen])))
          {
            badbound = true;
          }
        }
      }
      if (!badbound) {
        COLORER_LOG_DEEPTRACE("[TextParserImpl] KW matched. gx=%, region=%", gx,
                             node->kwList->kwList[pos].region->getName());
        addRegion(current_parse_line, gx, gx + kwlen, node->kwList->kwList[pos].region);
        gx += kwlen;
        return MATCH_RE;
      }
    }
    if (right - left == 1) {
      left = node->kwList->kwList[pos].indexOfShorter;
      if (left != -1) {
        right = left + 1;
        continue;
      }
      break;
    }
    if (compare_result == 1) {
      right = pos;
    }
    else {  // if (compare_result == 0 || compare_result == -1)
      left = pos;
    }
  }
  return MATCH_NOTHING;
}

int TextParser::Impl::searchIN(SchemeNodeInherit* node, int no, int lowLen, int hiLen)
{
  // Если для inherit схемы не задана реализация, то ничего не делаем.
  // Не задана т.к. HrcLibrary::Imp::updateLinks не нашла
  if (!node->scheme) {
    return MATCH_NOTHING;
  }

  int re_result = MATCH_NOTHING;
  // ищем для текущей схемы возможную замену через virtual предыдущих inherit
  SchemeImpl* ssubst = vtlist->pushvirt(node->scheme);
  if (!ssubst) {
    // не нашли замену
    // помещаем текущий inherit в список для будущих замен. True - если поместили, не было
    // ограничений
    bool b = vtlist->push(node);
    // парсим текст по имплементации текущего inherit
    re_result = searchMatch(node->scheme, no, lowLen, hiLen);
    if (b) {
      // достаем inherit из списка, больше он не нужен
      vtlist->pop();
    }
  }
  else {
    // нашли замену, по ней далее парсим текст
    re_result = searchMatch(ssubst, no, lowLen, hiLen);
    vtlist->popvirt();
  }
  return re_result;
}

int TextParser::Impl::searchRE(SchemeNodeRegexp* node, int /*no*/, int lowLen, int hiLen)
{
  SMatches match {};
  if (!node->start->parse(str, gx, node->lowPriority ? lowLen : hiLen, &match, schemeStart)) {
    return MATCH_NOTHING;
  }
  COLORER_LOG_DEEPTRACE("[TextParserImpl] RE matched. gx=%", gx);
  for (int i = 0; i < match.cMatch; i++) {
    addRegion(current_parse_line, match.s[i], match.e[i], node->regions[i]);
  }
  for (int i = 0; i < match.cnMatch; i++) {
    addRegion(current_parse_line, match.ns[i], match.ne[i], node->regionsn[i]);
  }

  /* skips regexp if it has zero length */
  // это случай использования \m \M, например, для пометки текста как outline
  if (match.e[0] == match.s[0]) {
    return MATCH_NOTHING;
  }
  // сдвигаем границу распознанного текста на конец найденного блока
  gx = match.e[0];
  return MATCH_RE;
}

int TextParser::Impl::searchBL(SchemeNodeBlock* node, int no, int lowLen, int hiLen)
{
  // Если для схемы блока не задана реализация, то ничего не делаем.
  // Не задана т.к. HrcLibrary::Imp::updateLinks не нашла
  if (!node->scheme) {
    return MATCH_NOTHING;
  }

  // проверяем совпадение по регулярному выражению start
  SMatches match {};
  if (!node->start->parse(str, gx, node->lowPriority ? lowLen : hiLen, &match, schemeStart)) {
    return MATCH_NOTHING;
  }

  // есть совпадение
  COLORER_LOG_DEEPTRACE("[TextParserImpl] Scheme matched. gx=%", gx);
  gx = match.e[0];
  // проверяем наличие замены через virtual для данной схемы
  SchemeImpl* ssubst = vtlist->pushvirt(node->scheme);
  if (!ssubst) {
    // замены нет, работаем с текущей
    ssubst = node->scheme;
  }

  ParseCache* OldCacheF = nullptr;
  ParseCache* OldCacheP = nullptr;
  ParseCache* ResF = nullptr;
  ParseCache* ResP = nullptr;

  auto* backLine = new UnicodeString(*str);
  if (updateCache) {
    ResF = forward;
    ResP = parent;
    if (forward) {
      forward->next = new ParseCache;
      forward->next->prev = forward;
      OldCacheF = forward->next;
      OldCacheP = parent ? parent : forward->parent;
      parent = forward->next;
      forward = nullptr;
    }
    else {
      forward = new ParseCache;
      parent->children = forward;
      OldCacheF = forward;
      OldCacheP = parent;
      parent = forward;
      forward = nullptr;
    }
    OldCacheF->parent = OldCacheP;
    OldCacheF->sline = current_parse_line + 1;
    OldCacheF->eline = 0x7FFFFFFF;
    OldCacheF->scheme = ssubst;
    OldCacheF->matchstart = match;
    OldCacheF->clender = node;
    OldCacheF->backLine = backLine;
  }

  // сохраняем текущие значения ...
  // .. переменных текущего экземпляра класса
  auto old_gy = current_parse_line;
  auto old_scheme = baseScheme;
  auto old_schemeStart = schemeStart;
  auto old_matchend = matchend;

  // ... переменных регулярного выражения end блока
  SMatches* old_reg_match;
  UnicodeString* old_reg_str;
  auto scheme_end = node->end.get();
  scheme_end->getBackTrace((const UnicodeString**) &old_reg_str, &old_reg_match);

  // задаем новые значения
  baseScheme = ssubst;
  schemeStart = gx;
  scheme_end->setBackTrace(backLine, &match);

  enterScheme(no, &match, node);
  colorize(scheme_end, node->lowContentPriority);

  if (current_parse_line < end_line4parse) {
    leaveScheme(current_parse_line, &matchend, node);
  }
  gx = matchend.e[0];
  /* (empty-block.test) Check if the consumed scheme is zero-length */
  bool zeroLength = (match.s[0] == matchend.e[0] && old_gy == current_parse_line);

  // восстанавливаем старые значения
  scheme_end->setBackTrace(old_reg_str, old_reg_match);
  matchend = old_matchend;
  schemeStart = old_schemeStart;
  baseScheme = old_scheme;

  if (updateCache) {
    if (old_gy == current_parse_line) {
      delete OldCacheF;
      if (ResF) {
        ResF->next = nullptr;
      }
      else if (ResP) {
        ResP->children = nullptr;
      }
      forward = ResF;
      parent = ResP;
    }
    else {
      OldCacheF->eline = current_parse_line;
      OldCacheF->vcache = vtlist->store();
      forward = OldCacheF;
      parent = OldCacheP;
    }
  }
  else {
    delete backLine;
  }
  if (ssubst != node->scheme) {
    vtlist->popvirt();
  }

  /* (empty-block.test) skips block if it has zero length and spread over single line */
  if (zeroLength) {
    return MATCH_NOTHING;
  }

  return MATCH_SCHEME;
}

int TextParser::Impl::searchMatch(const SchemeImpl* cscheme, int no, int lowLen, int hiLen)
{
  COLORER_LOG_DEEPTRACE("[TextParserImpl] searchMatch: entered scheme \"%\"", *cscheme->getName());

  if (!cscheme) {
    return MATCH_NOTHING;
  }
#ifdef COLORER_USE_DEEPTRACE
  int idx = 0;
#endif
  for (auto const& schemeNode : cscheme->nodes) {
    COLORER_LOG_DEEPTRACE("[TextParserImpl] searchMatch: processing node:%/%, type:%", idx + 1,
                         cscheme->nodes.size(),
                         SchemeNode::schemeNodeTypeNames[static_cast<int>(schemeNode->type)]);
    switch (schemeNode->type) {
      case SchemeNode::SchemeNodeType::SNT_INHERIT: {
        auto schemeNodeInherit = static_cast<SchemeNodeInherit*>(schemeNode.get());
        int re_result = searchIN(schemeNodeInherit, no, lowLen, hiLen);
        if (re_result != MATCH_NOTHING) {
          return re_result;
        }
        break;
      }
      case SchemeNode::SchemeNodeType::SNT_KEYWORDS: {
        auto schemeNodeKe = static_cast<SchemeNodeKeywords*>(schemeNode.get());
        if (searchKW(schemeNodeKe, no, lowLen, hiLen) == MATCH_RE) {
          return MATCH_RE;
        }
        break;
      }
      case SchemeNode::SchemeNodeType::SNT_RE: {
        auto schemeNodeRe = static_cast<SchemeNodeRegexp*>(schemeNode.get());
        if (searchRE(schemeNodeRe, no, lowLen, hiLen) == MATCH_RE) {
          return MATCH_RE;
        }
        break;
      }
      case SchemeNode::SchemeNodeType::SNT_BLOCK: {
        auto schemeNodeBlock = static_cast<SchemeNodeBlock*>(schemeNode.get());
        if (searchBL(schemeNodeBlock, no, lowLen, hiLen) != MATCH_NOTHING) {
          return MATCH_SCHEME;
        }
        break;
      }
    }
#ifdef COLORER_USE_DEEPTRACE
    idx++;
#endif
  }
  return MATCH_NOTHING;
}

bool TextParser::Impl::colorize(CRegExp* root_end_re, bool lowContentPriority)
{
  len = -1;

  /* Direct check for recursion level */
  if (stackLevel > MAX_RECURSION_LEVEL) {
    return true;
  }
  stackLevel++;

  for (; current_parse_line < end_line4parse;) {
    COLORER_LOG_DEEPTRACE("[TextParserImpl] colorize: line no %", current_parse_line);
    // clears line at start,
    // prevents multiple requests on each line
    if (clearLine != current_parse_line) {
      clearLine = current_parse_line;
      str = lineSource->getLine(current_parse_line);
      if (str == nullptr) {
        throw Exception("null String passed into the parser: " +
                        UStr::to_unistr(current_parse_line));
      }
      regionHandler->clearLine(current_parse_line, str);
    }
    // hack to include invisible regions in start of block
    // when parsing with cache information
    if (!invisibleSchemesFilled) {
      invisibleSchemesFilled = true;
      fillInvisibleSchemes(parent);
    }
    // updates length
    if (len < 0) {
      len = str->length();
    }
    endLine = current_parse_line;

    // searches for the end of parent block
    int res = 0;
    if (root_end_re) {
      res = root_end_re->parse(str, gx, len, &matchend, schemeStart);
    }
    if (!res) {
      matchend.s[0] = matchend.e[0] = gx + maxBlockSize > len ? len : gx + maxBlockSize;
    }

    int parent_len = len;
    /*
    BUG: <regexp match="/.{3}\M$/" region="def:Error" priority="low"/>
    $ at the end of current schema
    */
    if (lowContentPriority) {
      len = matchend.s[0];
    }

    int ret = LINE_NEXT;
    for (; gx <= matchend.s[0];) {  //    '<' or '<=' ???
      if (breakParsing) {
        current_parse_line = end_line4parse;
        break;
      }
      int old_current_parse_line = current_parse_line;
      int re_result =
          searchMatch(baseScheme, current_parse_line, matchend.s[0],
                      matchend.s[0] + maxBlockSize > len ? len : matchend.s[0] + maxBlockSize);
      if ((re_result == MATCH_SCHEME &&
           (old_current_parse_line != current_parse_line || matchend.s[0] < gx)) ||
          (re_result == MATCH_RE && matchend.s[0] < gx))
      {
        len = -1;
        ret = LINE_REPARSE;
        break;
      }
      if (old_current_parse_line == current_parse_line) {
        len = parent_len;
      }
      if (re_result == MATCH_NOTHING) {
        gx++;
      }
    }
    if (ret == LINE_REPARSE) {
      continue;
    }

    schemeStart = -1;
    if (res) {
      stackLevel--;
      return true;
    }
    len = -1;
    current_parse_line++;
    gx = 0;
  }
  stackLevel--;
  return true;
}

void TextParser::Impl::setMaxBlockSize(int max_block_size)
{
  maxBlockSize = max_block_size;
}
