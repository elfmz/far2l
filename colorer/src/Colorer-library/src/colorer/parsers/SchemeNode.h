#ifndef _COLORER_SCHEMENODE_H_
#define _COLORER_SCHEMENODE_H_

#include <vector>
#include <colorer/Common.h>
#include <colorer/Region.h>
#include <colorer/parsers/KeywordList.h>
#include <colorer/parsers/VirtualEntry.h>
#include <colorer/cregexp/cregexp.h>

extern const char* schemeNodeTypeNames[];

class SchemeImpl;
typedef std::vector<VirtualEntry*> VirtualEntryVector;

// Must be not less than MATCHES_NUM in cregexp.h
#define REGIONS_NUM MATCHES_NUM
#define NAMED_REGIONS_NUM NAMED_MATCHES_NUM

/** Scheme node.
    @ingroup colorer_parsers
*/
class SchemeNode
{
public:
  enum SchemeNodeType { SNT_EMPTY, SNT_RE, SNT_SCHEME, SNT_KEYWORDS, SNT_INHERIT };

  SchemeNodeType type;

  UString schemeName;
  SchemeImpl* scheme;

  VirtualEntryVector virtualEntryVector;
  std::unique_ptr<KeywordList> kwList;
  std::unique_ptr<CharacterClass> worddiv;

  const Region* region;
  const Region* regions[REGIONS_NUM];
  const Region* regionsn[NAMED_REGIONS_NUM];
  const Region* regione[REGIONS_NUM];
  const Region* regionen[NAMED_REGIONS_NUM];
  std::unique_ptr<CRegExp> start;
  std::unique_ptr<CRegExp> end;
  bool innerRegion;
  bool lowPriority;
  bool lowContentPriority;


  SchemeNode();
  ~SchemeNode();
};


#endif //_COLORER_SCHEMENODE_H_


