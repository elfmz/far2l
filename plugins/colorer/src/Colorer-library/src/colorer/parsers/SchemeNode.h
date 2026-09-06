#ifndef COLORER_SCHEMENODE_H
#define COLORER_SCHEMENODE_H

#include "colorer/Common.h"
#include "colorer/Region.h"
#include "colorer/cregexp/cregexp.h"
#include "colorer/parsers/KeywordList.h"
#include "colorer/parsers/VirtualEntry.h"
#include <vector>

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
  enum class SchemeNodeType { SNT_RE, SNT_BLOCK, SNT_KEYWORDS, SNT_INHERIT };
  static constexpr const char* schemeNodeTypeNames[] = {"RE", "BLOCK", "KEYWORDS", "INHERIT"};

  SchemeNodeType type;

  explicit SchemeNode(SchemeNodeType _type) : type(_type) {};
  virtual ~SchemeNode() = default;
};

class SchemeNodeInherit final : public SchemeNode
{
 public:
  uUnicodeString schemeName = nullptr;
  SchemeImpl* scheme = nullptr;
  VirtualEntryVector virtualEntryVector;
  SchemeNodeInherit() : SchemeNode(SchemeNodeType::SNT_INHERIT){};
  ~SchemeNodeInherit() override;
};

class SchemeNodeRegexp final : public SchemeNode
{
 public:
  bool lowPriority = false;
  std::unique_ptr<CRegExp> start;
  const Region* region = nullptr;
  const Region* regions[REGIONS_NUM] = {};
  const Region* regionsn[NAMED_REGIONS_NUM] = {};

  SchemeNodeRegexp() : SchemeNode(SchemeNodeType::SNT_RE) {};
  ~SchemeNodeRegexp() override = default;
};

class SchemeNodeBlock final : public SchemeNode
{
 public:
  bool innerRegion = false;
  bool lowPriority = false;
  bool lowContentPriority = false;
  uUnicodeString schemeName = nullptr;
  SchemeImpl* scheme = nullptr;
  std::unique_ptr<CRegExp> start;
  std::unique_ptr<CRegExp> end;
  const Region* region = nullptr;
  const Region* regions[REGIONS_NUM] = {};
  const Region* regionsn[NAMED_REGIONS_NUM] = {};
  const Region* regione[REGIONS_NUM] = {};
  const Region* regionen[NAMED_REGIONS_NUM] = {};

  SchemeNodeBlock() : SchemeNode(SchemeNodeType::SNT_BLOCK) {};
  ~SchemeNodeBlock() override = default;
};

class SchemeNodeKeywords final : public SchemeNode
{
 public:
  std::unique_ptr<KeywordList> kwList;
  std::unique_ptr<CharacterClass> worddiv;
  SchemeNodeKeywords() : SchemeNode(SchemeNodeType::SNT_KEYWORDS) {};
  ~SchemeNodeKeywords() override = default;
};

#endif  //COLORER_SCHEMENODE_H
