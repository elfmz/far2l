#ifndef _COLORER_TEXTPARSERIMPL_H_
#define _COLORER_TEXTPARSERIMPL_H_

#include<colorer/TextParser.h>
#include<colorer/parsers/TextParserHelpers.h>

#define MAX_RECURSION_LEVEL 100

/**
 * Implementation of TextParser interface.
 * This is the base Colorer syntax parser, which
 * works with parsed internal HRC structure and colorisez
 * text in a target editor system.
 * @ingroup colorer_parsers
 */
class TextParserImpl : public TextParser
{
public:
  TextParserImpl();
  ~TextParserImpl();

  void setFileType(FileType* type);

  void setLineSource(LineSource* lh);
  void setRegionHandler(RegionHandler* rh);

  int  parse(int from, int num, TextParseMode mode);
  void breakParse();
  void clearCache();
  void setMaxBlockSize(int max_block_size);
private:
  SString* str;
  int stackLevel;
  int gx, gy, gy2, len;
  int clearLine, endLine, schemeStart;
  SchemeImpl* baseScheme;

  bool breakParsing;
  bool first, invisibleSchemesFilled;
  bool drawing, updateCache;
  const Region* picked;

  ParseCache* cache;
  ParseCache* parent, *forward;

  int cachedLineNo;
  ParseCache* cachedParent, *cachedForward;

  SMatches matchend;
  VTList* vtlist;

  LineSource* lineSource;
  RegionHandler* regionHandler;
  // maximum block size of regexp in string line
  int maxBlockSize;

  void fillInvisibleSchemes(ParseCache* cache);
  void addRegion(int lno, int sx, int ex, const Region* region);
  void enterScheme(int lno, int sx, int ex, const Region* region);
  void leaveScheme(int lno, int sx, int ex, const Region* region);
  void enterScheme(int lno, SMatches* match, const SchemeNode* schemeNode);
  void leaveScheme(int lno, SMatches* match, const SchemeNode* schemeNode);

  int searchKW(const SchemeNode* node, int no, int lowLen, int hiLen);
  int searchRE(SchemeImpl* cscheme, int no, int lowLen, int hiLen);
  bool colorize(CRegExp* root_end_re, bool lowContentPriority);
};

#endif



