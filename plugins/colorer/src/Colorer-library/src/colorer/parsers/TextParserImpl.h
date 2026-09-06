#ifndef COLORER_TEXTPARSERIMPL_H
#define COLORER_TEXTPARSERIMPL_H

#include "colorer/TextParser.h"
#include "colorer/parsers/TextParserHelpers.h"

#define MAX_RECURSION_LEVEL 100

/**
 * Implementation of TextParser interface.
 * This is the base Colorer syntax parser, which
 * works with parsed internal HRC structure and colorisez
 * text in a target editor system.
 *
 * Hot path: parse() walks ParseCache to the scheme covering `from`
 * (searchLine resumes from a per-parent sibling cursor), then
 * colorize() on each line. One scheme is active; gx is the column.
 * Per line the ASCII occupancy mask (str_chars) is computed once and
 * passed into every CRegExp::mayMatch/parse. searchMatch() uses the
 * scheme's searchDispatch (first-character candidate list) then tries
 * keywords / regexp / block / inherit in HRC order. A block start-RE
 * recurses into colorize(end-RE). Multi-line blocks become ParseCache
 * nodes; single-line matches do not.
 *
 * tryParseLine() reparses one line under TPM_CACHE_READ and keeps the
 * rest of the file valid if the open-block stack (scheme, start-RE
 * captures) still matches what the cache predicted for the next line.
 *
 * @ingroup colorer_parsers
 */
class TextParser::Impl
{
 public:
  Impl();
  ~Impl();

  void setFileType(FileType* type);
  void setLineSource(LineSource* lh);
  void setRegionHandler(RegionHandler* rh);
  int parse(int from, int num, TextParseMode mode);
  bool tryParseLine(int line);
  FileType* currentFileType() const;
  void breakParse();
  void initCache();
  void setMaxBlockSize(int max_block_size);
  void setChunkLongLines(bool chunk);
  bool getChunkLongLines() const;

 private:
  struct TryLevel
  {
    SchemeImpl* scheme = nullptr;
    const SchemeNodeBlock* clender = nullptr;
    SMatches matchstart = {};
    UnicodeString startText;
  };
  UnicodeString* str = nullptr;
  UnicodeString str_lowercase;
  bool str_lowercase_ready = false;
  // ASCII characters present in str; lets CRegExp skip patterns that cannot match the line.
  AsciiCharMask str_chars = {};
  int stackLevel = 0;
  int current_parse_line = 0;
  int gx = 0;
  int end_line4parse = 0;
  int len = -1;
  int clearLine = -1;
  int endLine = 0;
  int schemeStart = -1;
  SchemeImpl* baseScheme = nullptr;

  bool breakParsing = false;
  bool invisibleSchemesFilled = false;
  bool updateCache = false;
  bool tracingTry = false;
  bool tryMismatch = false;
  std::vector<TryLevel> tryStack;

  ParseCache* cache = nullptr;
  ParseCache* parent = nullptr;
  ParseCache* forward = nullptr;

  SMatches matchend = {};
  VTList vtlist;

  LineSource* lineSource = nullptr;
  RegionHandler* regionHandler = nullptr;

  // maximum block size of regexp in string line
  int maxBlockSize = 1000;
  bool chunkLongLines = false;

  void fillInvisibleSchemes(ParseCache* cache);
  void addRegion(int lno, int sx, int ex, const Region* region);
  void enterScheme(int lno, int sx, int ex, const Region* region);
  void leaveScheme(int lno, int sx, int ex, const Region* region);
  void enterScheme(int lno, const SMatches* match, const SchemeNodeBlock* schemeNode);
  void leaveScheme(int, const SMatches* match, const SchemeNodeBlock* schemeNode);

  void ensureLineLowercase();
  int searchKW(const SchemeNodeKeywords* node, int, int lowlen, int);
  int searchIN(SchemeNodeInherit* node, int no, int lowLen, int hiLen);
  int searchRE(SchemeNodeRegexp* node, int no, int lowLen, int hiLen);
  int searchBL(SchemeNodeBlock* node, int no, int lowLen, int hiLen);
  int searchMatch(const SchemeImpl* cscheme, int no, int lowLen, int hiLen);
  bool colorize(CRegExp* root_end_re, bool lowContentPriority, bool tryPopOnEnd = false);
  void collectOpenLevels(ParseCache* node, std::vector<TryLevel>& out) const;
  static bool sameTryLevel(const TryLevel& a, const TryLevel& b);
  static UnicodeString matchSpan(const UnicodeString* line, const SMatches& match);
};

#endif // COLORER_TEXTPARSERIMPL_H
